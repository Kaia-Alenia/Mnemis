#include <gtest/gtest.h>
#include "core/indexer/Indexer.hpp"
#include "core/indexer/IndexerProgressObserver.hpp"
#include "database/DatabaseConnection.hpp"
#include "database/MigrationManager.hpp"
#include "database/repositories/SQLiteMediaRepository.hpp"
#include "filesystem/StdFileSystem.hpp"
#include "indexer/CompositeMetadataExtractor.hpp"
#include "indexer/StbImageExtractor.hpp"
#include "indexer/TagLibExtractor.hpp"
#include "indexer/FFmpegExtractor.hpp"
#include "core/models/MediaItem.hpp"
#include "core/ILogger.hpp"

#include <filesystem>
#include <memory>
#include <vector>

using namespace mnemis::core;
using namespace mnemis::core::indexer;
using namespace mnemis::core::models;
using namespace mnemis::database;
using namespace mnemis::filesystem;
using namespace mnemis::database::repositories;
using namespace mnemis::indexer;

const std::string TEST_DB_PATH = std::string(TEST_FIXTURE_DIR) + "/test_indexer.db";
const std::string MEDIA_DIR = std::string(TEST_FIXTURE_DIR) + "/media/";

class DummyLogger : public mnemis::core::ILogger {
public:
    void log(mnemis::core::LogLevel level, std::string_view message) override {}
};

class DummyObserver : public IndexerProgressObserver {
public:
    bool onDiscovered(const std::string& path) override { return true; }
    bool onProcessed(const MediaItem& item) override { return true; }
    bool onSkipped(const std::string& path) override { return true; }
    bool onError(const std::string& path, const std::string& err) override { return true; }
};

class IndexerIntegrationTest : public ::testing::Test {
protected:
    std::shared_ptr<DatabaseConnection> conn;
    std::shared_ptr<StdFileSystem> fs;
    std::shared_ptr<SQLiteMediaRepository> repo;
    std::unique_ptr<Indexer> indexer;
    std::shared_ptr<DummyLogger> logger;

    void SetUp() override {
        if (std::filesystem::exists(TEST_DB_PATH)) {
            std::filesystem::remove(TEST_DB_PATH);
        }

        logger = std::make_shared<DummyLogger>();

        conn = std::make_shared<DatabaseConnection>();
        conn->open(TEST_DB_PATH);

        MigrationManager migrator(*conn, *logger);
        migrator.runMigrations();

        repo = std::make_shared<SQLiteMediaRepository>(*conn);
        fs = std::make_shared<StdFileSystem>(logger.get());

        auto composite = std::make_unique<CompositeMetadataExtractor>();
        composite->addExtractor(std::make_unique<StbImageExtractor>());
        composite->addExtractor(std::make_unique<TagLibExtractor>());
        composite->addExtractor(std::make_unique<FFmpegExtractor>());

        indexer = std::make_unique<Indexer>(fs, repo, std::move(composite));
    }

    void TearDown() override {
        indexer.reset();
        repo.reset();
        conn.reset();
        
        if (std::filesystem::exists(TEST_DB_PATH)) {
            std::filesystem::remove(TEST_DB_PATH);
        }
    }
};

TEST_F(IndexerIntegrationTest, New_Modified_Lifecycle) {
    // 1. Initial Indexing
    auto res = indexer->indexDirectory(MEDIA_DIR);
    if(!res.isSuccess()) std::cout << "ERROR: " << res.error().message << std::endl; ASSERT_TRUE(res.isSuccess());
    
    auto all_media_res = repo->list(0, 100);
    if(!all_media_res.isSuccess()) std::cout << "ERROR: " << all_media_res.error().message << std::endl; ASSERT_TRUE(all_media_res.isSuccess());
    
    size_t count = all_media_res.value().size();
    EXPECT_GT(count, 5); 
    
    bool found_audio = false;
    for(const auto& item : all_media_res.value()) {
        if (item.fileName == "test_audio.mp3") {
            found_audio = true;
            EXPECT_EQ(item.title.value_or(""), "Test Title");
            EXPECT_EQ(item.artist.value_or(""), "Test Artist");
        }
    }
    EXPECT_TRUE(found_audio);

    // 2. Unchanged 
    auto scan2_res = indexer->indexDirectory(MEDIA_DIR);
    ASSERT_TRUE(scan2_res.isSuccess());
    
    auto all_media_res2 = repo->list(0, 100);
    ASSERT_TRUE(all_media_res2.isSuccess());
    EXPECT_EQ(all_media_res2.value().size(), count);

    // 3. Modifying a file
    std::filesystem::path p = MEDIA_DIR + "test_audio.mp3";
    auto ftime = std::filesystem::last_write_time(p);
    std::filesystem::last_write_time(p, ftime + std::chrono::hours(1));

    auto scan3_res = indexer->indexDirectory(MEDIA_DIR);
    ASSERT_TRUE(scan3_res.isSuccess());

    auto all_media_res3 = repo->list(0, 100);
    ASSERT_TRUE(all_media_res3.isSuccess());
    EXPECT_EQ(all_media_res3.value().size(), count);
}

class CancelObserver : public IndexerProgressObserver {
public:
    int processedCount = 0;
    bool onDiscovered(const std::string& path) override { return false; /* cancel immediately */ }
    bool onProcessed(const MediaItem& item) override { processedCount++; return false; }
    bool onSkipped(const std::string& path) override { return false; }
    bool onError(const std::string& path, const std::string& err) override { return false; }
};

TEST_F(IndexerIntegrationTest, Cancellation) {
    CancelObserver observer;
    auto res = indexer->indexDirectory(MEDIA_DIR, &observer);
    
    auto all_media_res = repo->list(0, 100);
    if(!all_media_res.isSuccess()) std::cout << "ERROR: " << all_media_res.error().message << std::endl; ASSERT_TRUE(all_media_res.isSuccess());
    
    // Check it processed 0 or very few files due to cancellation
    EXPECT_EQ(all_media_res.value().size(), 0);
}

TEST_F(IndexerIntegrationTest, MissingFileHandling) {
    MediaItem item;
    item.path = MEDIA_DIR + "nonexistent.mp3";
    item.canonicalPath = MEDIA_DIR + "nonexistent.mp3";
    item.fileName = "nonexistent.mp3";
    item.fileSize = 1234;
    item.modifiedTime = 0;
    
    repo->add(item);
    
    indexer->indexDirectory(MEDIA_DIR);
    
    auto all_items = repo->list(0, 100);
    bool found = false;
    for(const auto& i : all_items.value()) {
        if (i.fileName == "nonexistent.mp3") found = true;
    }
    EXPECT_FALSE(found) << "Missing files should be cleaned up by reconciliation";
}

TEST_F(IndexerIntegrationTest, StressTest_1000Files) {
    const std::string stress_dir = MEDIA_DIR + "stress/";
    std::filesystem::create_directories(stress_dir);
    for(int i = 0; i < 1000; ++i) {
        std::string name = stress_dir + "file_" + std::to_string(i) + ".mp3";
        FILE* f = fopen(name.c_str(), "w");
        if(f) {
            fputs("dummy", f);
            fclose(f);
        }
    }
    
    auto res = indexer->indexDirectory(stress_dir);
    if(!res.isSuccess()) std::cout << "ERROR: " << res.error().message << std::endl; ASSERT_TRUE(res.isSuccess());
    
    auto count_res = repo->list(0, 2000);
    ASSERT_TRUE(count_res.isSuccess());
    
    int found_stress_files = 0;
    for(const auto& item : count_res.value()) {
        if (item.fileName.find("file_") == 0) {
            found_stress_files++;
        }
    }
    EXPECT_EQ(found_stress_files, 1000);
    
    std::filesystem::remove_all(stress_dir);
}

TEST_F(IndexerIntegrationTest, PathBoundaryReconciliation) {
    // We want to test that indexing /Music does NOT delete files from /MusicBackup
    const std::string rootDir = MEDIA_DIR + "Music/";
    const std::string backupDir = MEDIA_DIR + "MusicBackup/";

    std::filesystem::create_directories(rootDir);
    std::filesystem::create_directories(backupDir);

    // Add a file physically to Music and MusicBackup so we can index one
    std::string fMusic = rootDir + "test1.mp3";
    std::string fBackup = backupDir + "test2.mp3";
    
    FILE* f1 = fopen(fMusic.c_str(), "w");
    if (f1) { fputs("dummy", f1); fclose(f1); }
    FILE* f2 = fopen(fBackup.c_str(), "w");
    if (f2) { fputs("dummy", f2); fclose(f2); }

    // Manually add the backup file to DB
    MediaItem itemBackup;
    itemBackup.path = fBackup;
    itemBackup.canonicalPath = std::filesystem::canonical(fBackup).string();
    itemBackup.fileName = "test2.mp3";
    repo->add(itemBackup);

    // Index /Music
    auto res = indexer->indexDirectory(rootDir);
    if(!res.isSuccess()) std::cout << "ERROR: " << res.error().message << std::endl; ASSERT_TRUE(res.isSuccess());

    // Verify
    auto all_items = repo->list(0, 100);
    bool foundMusic = false;
    bool foundBackup = false;
    for (const auto& i : all_items.value()) {
        if (i.fileName == "test1.mp3") foundMusic = true;
        if (i.fileName == "test2.mp3") foundBackup = true;
    }
    
    EXPECT_TRUE(foundMusic) << "Indexed file from /Music should be in DB";
    EXPECT_TRUE(foundBackup) << "Reconciliation of /Music should NOT delete files in /MusicBackup";

    std::filesystem::remove_all(rootDir);
    std::filesystem::remove_all(backupDir);
}

TEST_F(IndexerIntegrationTest, CancelledScanReconciliation) {
    // If scan is cancelled, reconciliation should be skipped.
    // So a missing file should NOT be removed.
    
    MediaItem item;
    item.path = MEDIA_DIR + "nonexistent_cancelled.mp3";
    try {
        item.canonicalPath = std::filesystem::canonical(MEDIA_DIR).string() + "/nonexistent_cancelled.mp3";
    } catch (...) {
        item.canonicalPath = MEDIA_DIR + "nonexistent_cancelled.mp3";
    }
    item.fileName = "nonexistent_cancelled.mp3";
    repo->add(item);
    
    CancelObserver observer;
    auto res = indexer->indexDirectory(MEDIA_DIR, &observer);
    
    auto all_items = repo->list(0, 100);
    bool found = false;
    for(const auto& i : all_items.value()) {
        if (i.fileName == "nonexistent_cancelled.mp3") found = true;
    }
    EXPECT_TRUE(found) << "Missing file should NOT be removed because scan was cancelled";
}

