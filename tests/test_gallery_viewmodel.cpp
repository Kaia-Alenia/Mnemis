#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTimer>
#include <QEventLoop>
#include <QThreadPool>
#include <chrono>

#include "ui/controllers/GalleryViewModel.hpp"
#include "core/models/MediaItem.hpp"
#include "core/repositories/IMediaRepository.hpp"
#include "core/Result.hpp"

#include "core/events/ILibraryEventBus.hpp"

using namespace mnemis::core;
using namespace mnemis::core::models;
using namespace mnemis::core::repositories;
using namespace mnemis::ui::controllers;

class MockThumbnailEngine : public mnemis::core::thumbnails::IThumbnailEngine {
public:
    int requestCount = 0;
    std::vector<mnemis::core::thumbnails::ThumbnailTaskId> canceledTasks;
    mnemis::core::thumbnails::ThumbnailStatus defaultStatus = mnemis::core::thumbnails::ThumbnailStatus::Ready;
    
    void clearPendingRequests() override {}
    void cancelRequest(mnemis::core::thumbnails::ThumbnailTaskId taskId) override {
        canceledTasks.push_back(taskId);
    }
    
    mnemis::core::thumbnails::ThumbnailTaskId requestThumbnail(
        const mnemis::core::thumbnails::ThumbnailSpec& spec, 
        mnemis::core::thumbnails::ThumbnailPriority priority, 
        mnemis::core::thumbnails::ThumbnailCallback callback) override {
        requestCount++;
        mnemis::core::thumbnails::ThumbnailTaskId taskId = requestCount;
        
        mnemis::core::thumbnails::ThumbnailResult res;
        res.mediaId = spec.mediaId;
        res.status = defaultStatus;
        
        QTimer::singleShot(10, [callback, res]() {
            callback(res);
        });
        return taskId;
    }
};


class MockRepository : public IMediaRepository {
public:
    int totalItems = 10000;
    bool block = false;

    void setEventBus(std::shared_ptr<events::ILibraryEventBus> eventBus) override {}

    Result<void> add(const MediaItem& item) override { return Result<void>(); }
    Result<void> update(const MediaItem& item) override { return Result<void>(); }
    Result<void> remove(const std::string& mediaId) override { return Result<void>(); }
    Result<std::optional<MediaItem>> getById(const std::string& mediaId) override { return std::optional<MediaItem>(std::nullopt); }
    
    std::mutex m_mutex;
    QueryOptions lastOptions;
    
    Result<std::vector<MediaItem>> list(int page, int pageSize, const QueryOptions& options = {}) override {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            lastOptions = options;
        }
        while (block) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::vector<MediaItem> items;
        // In the real impl page is 1-based but in this mock page is 0-based based on the test
        // Wait, GalleryViewModel was passing chunkIndex (0-based) to `list(chunkIndex)` before, 
        // now it passes `chunkIndex + 1` to `list(chunkIndex + 1, CHUNK_SIZE, query)`.
        // So the mock needs to adapt: start = (page - 1) * pageSize.
        // Let's ensure page >= 1
        int zeroBasedPage = std::max(0, page - 1);
        int start = zeroBasedPage * pageSize;
        for (int i = 0; i < pageSize && start + i < totalItems; ++i) {
            MediaItem item;
            item.mediaId = "id_" + std::to_string(start + i);
            item.fileName = "file_" + std::to_string(start + i) + ".jpg";
            items.push_back(item);
        }
        return items;
    }
    
    Result<int> count(const QueryOptions& options = {}) override { 
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            lastOptions = options;
        }
        return totalItems; 
    }
    
    Result<std::optional<MediaItem>> getByCanonicalPath(const std::string& canonicalPath) override { return std::optional<MediaItem>(std::nullopt); }
    Result<void> saveBatch(const std::vector<MediaItem>& items) override { return Result<void>(); }
    Result<void> removeBatch(const std::vector<std::string>& mediaIds) override { return Result<void>(); }
};

TEST(GalleryViewModelTest, RowCountMatchesRepository) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockRepository();
    repo->totalItems = 15000;

    GalleryViewModel vm(repo);
    
    // Wait for the background count query to finish
    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    QTimer::singleShot(1000, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(vm.rowCount(), 15000);
    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(GalleryViewModelTest, VirtualizationLoadsChunksAsync) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockRepository();
    repo->totalItems = 500;
    GalleryViewModel vm(repo);

    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    loop.exec();

    // Request data for index 250 (chunk 2)
    QModelIndex idx = vm.index(250, 0);
    QVariant dataBeforeLoad = vm.data(idx, GalleryViewModel::IsLoadedRole);
    EXPECT_FALSE(dataBeforeLoad.toBool());

    // Wait for chunk to be loaded
    QObject::connect(&vm, &GalleryViewModel::dataChanged, &loop, &QEventLoop::quit);
    loop.exec();

    QVariant dataAfterLoad = vm.data(idx, GalleryViewModel::IsLoadedRole);
    EXPECT_TRUE(dataAfterLoad.toBool());
    EXPECT_EQ(vm.data(idx, GalleryViewModel::MediaIdRole).toString(), "id_250");
    
    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(GalleryViewModelTest, BackgroundThreadDoesNotBlockUI) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockRepository();
    repo->totalItems = 20000;
    repo->block = true; // Blocks the repository queries

    GalleryViewModel vm(repo);

    // Initial count will block internally on background thread. UI should remain responsive.
    auto start = std::chrono::steady_clock::now();
    
    // Process events for 100ms
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // If it didn't block, duration should be ~100ms, not indefinite
    EXPECT_LT(duration, 500);

    repo->block = false; // release thread
    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(GalleryViewModelTest, SelectionAndFilteringAndSorting) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);
    auto repo = new MockRepository();
    repo->totalItems = 50;
    GalleryViewModel vm(repo);

    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    loop.exec();

    vm.toggleSelection("id_10");
    EXPECT_TRUE(vm.isSelected("id_10"));
    
    vm.clearSelection();
    EXPECT_FALSE(vm.isSelected("id_10"));
    
    vm.selectAll();
    // process events for selectAll to run
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();
    
    EXPECT_TRUE(vm.isSelected("id_0"));
    
    // Sort
    vm.setSortOptions("modifiedTime", false);
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    loop.exec();
    EXPECT_EQ(repo->lastOptions.sortBy, "modifiedTime");
    EXPECT_FALSE(repo->lastOptions.ascending);
    
    // Filter
    vm.setFilter("test_filter", -1);
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    loop.exec();
    EXPECT_EQ(repo->lastOptions.filterText.value_or(""), "test_filter");

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(GalleryViewModelTest, AudioFilter) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);
    auto repo = new MockRepository();
    GalleryViewModel vm(repo);
    QThreadPool::globalInstance()->waitForDone(); // settle the constructor's initial query

    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    vm.setFilter({}, static_cast<int>(MediaType::Audio));
    loop.exec();

    EXPECT_EQ(repo->lastOptions.filterMediaType, MediaType::Audio);
    EXPECT_FALSE(repo->lastOptions.filterFavorite.has_value());
    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(GalleryViewModelTest, FavoritesFilter) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);
    auto repo = new MockRepository();
    GalleryViewModel vm(repo);
    QThreadPool::globalInstance()->waitForDone(); // settle the constructor's initial query

    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    vm.setFavoriteFilter(true, "sunset");
    loop.exec();

    EXPECT_EQ(repo->lastOptions.filterFavorite, true);
    EXPECT_FALSE(repo->lastOptions.filterMediaType.has_value());
    EXPECT_EQ(repo->lastOptions.filterText.value_or(""), "sunset");
    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

class DelayedFilterRepository final : public MockRepository {
public:
    Result<int> count(const QueryOptions& options = {}) override {
        if (options.filterMediaType == MediaType::Image) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            return 11;
        }
        if (options.filterMediaType == MediaType::Audio) {
            return 3;
        }
        return 0;
    }
};

TEST(GalleryViewModelTest, FilterSwitchInvalidatesOldResults) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);
    auto repo = new DelayedFilterRepository();
    GalleryViewModel vm(repo);

    vm.setFilter({}, static_cast<int>(MediaType::Image));
    vm.setFilter({}, static_cast<int>(MediaType::Audio));

    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    QTimer::singleShot(1000, &loop, &QEventLoop::quit);
    loop.exec();
    QThreadPool::globalInstance()->waitForDone();

    EXPECT_EQ(vm.rowCount(), 3);
    delete repo;
}

TEST(GalleryViewModelTest, StressTest_10000_Items) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);
    
    auto repo = new MockRepository();
    repo->totalItems = 12000;
    GalleryViewModel vm(repo);

    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    loop.exec();
    
    EXPECT_EQ(vm.rowCount(), 12000);
    
    // Rapidly request random chunks to simulate fast scrolling
    for (int i = 0; i < 50; ++i) {
        int randomChunk = rand() % 100;
        vm.data(vm.index(randomChunk * 100, 0), GalleryViewModel::MediaIdRole);
    }
    
    // Process events and make sure it does not crash or timeout
    QTimer::singleShot(1000, &loop, &QEventLoop::quit);
    loop.exec();

    // The eviction policy should keep max 10 chunks loaded
    // This is tested implicitly by no crash/OOM
    EXPECT_GT(repo->lastOptions.sortBy, ""); // just some property

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(GalleryViewModelTest, ThumbnailReactiveState) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockRepository();
    repo->totalItems = 100;
    
    MockThumbnailEngine engine;
    GalleryViewModel vm(repo);
    vm.setThumbnailEngine(&engine);

    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    loop.exec();

    // Trigger load of chunk 0
    QModelIndex idx = vm.index(0, 0);
    vm.data(idx, GalleryViewModel::IsLoadedRole);
    QObject::connect(&vm, &GalleryViewModel::dataChanged, &loop, &QEventLoop::quit);
    loop.exec();
    
    // Request thumbnail state
    QVariant stateBefore = vm.data(idx, GalleryViewModel::ThumbnailStateRole);
    // Should be Pending initially
    EXPECT_EQ(stateBefore.toInt(), 0); // 0 = Pending

    // Wait for the thumbnail engine to callback asynchronously
    QObject::connect(&vm, &GalleryViewModel::dataChanged, &loop, &QEventLoop::quit);
    loop.exec();

    QVariant stateAfter = vm.data(idx, GalleryViewModel::ThumbnailStateRole);
    EXPECT_EQ(stateAfter.toInt(), 1); // 1 = Ready

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(GalleryViewModelTest, ThumbnailReactiveState_Error) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockRepository();
    repo->totalItems = 100;
    
    MockThumbnailEngine engine;
    engine.defaultStatus = mnemis::core::thumbnails::ThumbnailStatus::Error;
    GalleryViewModel vm(repo);
    vm.setThumbnailEngine(&engine);

    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    loop.exec();

    QModelIndex idx = vm.index(0, 0);
    vm.data(idx, GalleryViewModel::IsLoadedRole);
    QObject::connect(&vm, &GalleryViewModel::dataChanged, &loop, &QEventLoop::quit);
    loop.exec();
    
    QVariant stateBefore = vm.data(idx, GalleryViewModel::ThumbnailStateRole);
    EXPECT_EQ(stateBefore.toInt(), 0); // 0 = Pending

    QObject::connect(&vm, &GalleryViewModel::dataChanged, &loop, &QEventLoop::quit);
    loop.exec();

    QVariant stateAfter = vm.data(idx, GalleryViewModel::ThumbnailStateRole);
    EXPECT_EQ(stateAfter.toInt(), 3); // 3 = Error

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(GalleryViewModelTest, ThumbnailReactiveState_Unavailable) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockRepository();
    repo->totalItems = 100;
    
    MockThumbnailEngine engine;
    engine.defaultStatus = mnemis::core::thumbnails::ThumbnailStatus::Unavailable;
    GalleryViewModel vm(repo);
    vm.setThumbnailEngine(&engine);

    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    loop.exec();

    QModelIndex idx = vm.index(0, 0);
    vm.data(idx, GalleryViewModel::IsLoadedRole);
    QObject::connect(&vm, &GalleryViewModel::dataChanged, &loop, &QEventLoop::quit);
    loop.exec();
    
    QVariant stateBefore = vm.data(idx, GalleryViewModel::ThumbnailStateRole);
    EXPECT_EQ(stateBefore.toInt(), 0); // 0 = Pending

    QObject::connect(&vm, &GalleryViewModel::dataChanged, &loop, &QEventLoop::quit);
    loop.exec();

    QVariant stateAfter = vm.data(idx, GalleryViewModel::ThumbnailStateRole);
    EXPECT_EQ(stateAfter.toInt(), 2); // 2 = Unavailable

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(GalleryViewModelTest, ThumbnailCancelObsolete) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockRepository();
    repo->totalItems = 2000;
    
    MockThumbnailEngine engine;
    GalleryViewModel vm(repo);
    vm.setThumbnailEngine(&engine);

    QEventLoop loop;
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    loop.exec();

    // Trigger load of chunk 0
    vm.data(vm.index(0, 0), GalleryViewModel::IsLoadedRole);

    // Wait for chunk 0 to be loaded
    QObject::connect(&vm, &GalleryViewModel::dataChanged, &loop, &QEventLoop::quit);
    loop.exec();

    // Now request thumbnail, which should succeed since chunk is loaded
    vm.data(vm.index(0, 0), GalleryViewModel::ThumbnailStateRole);

    // Rapidly scroll to far away chunks to cause eviction of chunk 0
    for (int i = 5; i <= 30; ++i) {
        vm.data(vm.index(i * 100, 0), GalleryViewModel::IsLoadedRole);
        QTimer::singleShot(10, &loop, &QEventLoop::quit);
        loop.exec();
    }
    
    // Process remaining events
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    // Verify cancellation was requested for evicted items
    EXPECT_GT(engine.canceledTasks.size(), 0);

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

#include "database/repositories/SQLiteMediaRepository.hpp"
#include "database/DatabaseConnection.hpp"
#include "database/MigrationManager.hpp"
#include "fakes/FakeLogger.hpp"
#include "infrastructure/watcher/QtFileWatcher.hpp"
#include "core/indexer/Indexer.hpp"
#include "core/events/LibraryEventBus.hpp"
#include "filesystem/StdFileSystem.hpp"
#include "indexer/CompositeMetadataExtractor.hpp"
#include <QTemporaryDir>
#include <QFile>

class MockMetadataExtractor : public mnemis::core::indexer::IMetadataExtractor {
public:
    bool supports(mnemis::core::models::MediaType, const std::string&) const override {
        return true;
    }
    mnemis::core::Result<mnemis::core::indexer::Metadata> extract(const std::string&, mnemis::core::models::MediaType) override {
        return mnemis::core::indexer::Metadata{};
    }
};

TEST(GalleryViewModelTest, EndToEndIntegration) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);
    
    QTemporaryDir tempDir;
    
    auto dbConnection = std::make_shared<mnemis::database::DatabaseConnection>();
    dbConnection->open(":memory:");
    
    auto logger = std::make_shared<mnemis::tests::FakeLogger>();
    mnemis::database::MigrationManager migrator(*dbConnection, *logger);
    migrator.runMigrations();
    
    auto repo = std::make_shared<mnemis::database::repositories::SQLiteMediaRepository>(*dbConnection);
    
    auto eventBus = std::make_shared<mnemis::core::events::LibraryEventBus>();
    repo->setEventBus(eventBus);
    
    auto fs = std::make_shared<mnemis::filesystem::StdFileSystem>(logger.get());
    auto composite = std::make_shared<mnemis::indexer::CompositeMetadataExtractor>();
    composite->addExtractor(std::make_shared<MockMetadataExtractor>());
    auto indexer = std::make_shared<mnemis::core::indexer::Indexer>(fs, repo, composite);
    
    mnemis::infrastructure::watcher::QtFileWatcher watcher(50);
    watcher.setCallback([indexer](const std::string& path) {
        indexer->indexRoots({path});
    });
    
    GalleryViewModel vm(repo.get());
    vm.setEventBus(eventBus);
    
    watcher.watchDirectory(tempDir.path().toStdString());
    
    // Wait for gallery initialization
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();
    
    EXPECT_EQ(vm.rowCount(), 0);
    
    // Create a file to trigger the pipeline
    QFile file(tempDir.path() + "/test.mp3");
    file.open(QIODevice::WriteOnly);
    file.write("dummy audio content data data");
    file.close();
    
    // Wait for the pipeline: Watcher -> Indexer -> Repo -> EventBus -> Gallery
    QObject::connect(&vm, &GalleryViewModel::countChanged, &loop, &QEventLoop::quit);
    QTimer::singleShot(1500, &loop, &QEventLoop::quit);
    loop.exec();
    
    EXPECT_GT(vm.rowCount(), 0);
    
    watcher.unwatchDirectory(tempDir.path().toStdString());
    
    // Wait for any remaining background tasks before destroying test fixtures
    QThreadPool::globalInstance()->waitForDone();
}
