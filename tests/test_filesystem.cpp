#include <gtest/gtest.h>
#include "filesystem/StdFileSystem.hpp"
#include "fakes/FakeLogger.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

class FileSystemTest : public ::testing::Test {
protected:
    fs::path testDir;
    std::shared_ptr<mnemis::tests::FakeLogger> logger;
    std::unique_ptr<mnemis::filesystem::StdFileSystem> fileSystem;

    void SetUp() override {
        testDir = fs::temp_directory_path() / ("mnemis_fs_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directories(testDir);
        logger = std::make_shared<mnemis::tests::FakeLogger>();
        fileSystem = std::make_unique<mnemis::filesystem::StdFileSystem>(logger);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(testDir, ec);
    }

    void createFile(const fs::path& p, const std::string& content = "dummy") {
        std::ofstream f(p, std::ios::binary);
        f << content;
    }

    // Helper for Unicode paths
    fs::path u8path(const std::string& s) {
        return fs::path(std::u8string(s.begin(), s.end()));
    }
};

TEST_F(FileSystemTest, DiscoveryAndNestedDirectories) {
    fs::create_directories(testDir / "nested" / "deep");
    createFile(testDir / "file1.mp3");
    createFile(testDir / "nested" / "file2.FLAC");
    createFile(testDir / "nested" / "deep" / "file3.WAV");

    std::vector<mnemis::core::filesystem::FileInfo> discovered;
    auto res = fileSystem->scanDirectory(
        testDir.string(), 
        [&](const mnemis::core::filesystem::FileInfo& info) {
            discovered.push_back(info);
            return true;
        }
    );

    EXPECT_TRUE(res.isSuccess());
    EXPECT_EQ(discovered.size(), 3);
    
    // Check extension normalization
    bool foundFlac = false;
    for (const auto& f : discovered) {
        if (f.fileName == "file2.FLAC") {
            EXPECT_EQ(f.extension, ".flac");
            foundFlac = true;
        }
    }
    EXPECT_TRUE(foundFlac);
}

TEST_F(FileSystemTest, UnicodeSupport) {
    std::string unicodeName = reinterpret_cast<const char*>(u8"canción_ñ_japonés_こんにちは.mp3");
    std::string unicodeDir = reinterpret_cast<const char*>(u8"Música_🎵");
    
    fs::path uDir = testDir / u8path(unicodeDir);
    fs::create_directories(uDir);
    createFile(uDir / u8path(unicodeName));

    std::vector<mnemis::core::filesystem::FileInfo> discovered;
    auto res = fileSystem->scanDirectory(
        uDir.string(), 
        [&](const mnemis::core::filesystem::FileInfo& info) {
            discovered.push_back(info);
            return true;
        }
    );

    EXPECT_TRUE(res.isSuccess());
    ASSERT_EQ(discovered.size(), 1);
    EXPECT_EQ(discovered[0].fileName, unicodeName);
    EXPECT_EQ(discovered[0].extension, ".mp3");
}

TEST_F(FileSystemTest, SymlinkIgnored) {
#if defined(__unix__) || defined(__APPLE__)
    createFile(testDir / "target.mp3");
    symlink((testDir / "target.mp3").c_str(), (testDir / "link.mp3").c_str());
    
    std::vector<mnemis::core::filesystem::FileInfo> discovered;
    fileSystem->scanDirectory(
        testDir.string(), 
        [&](const mnemis::core::filesystem::FileInfo& info) {
            discovered.push_back(info);
            return true;
        }
    );

    EXPECT_EQ(discovered.size(), 1);
    EXPECT_EQ(discovered[0].fileName, "target.mp3");
#endif
}

TEST_F(FileSystemTest, SymlinkCycleIgnored) {
#if defined(__unix__) || defined(__APPLE__)
    symlink((testDir).c_str(), (testDir / "cycle_link").c_str());
    
    std::vector<mnemis::core::filesystem::FileInfo> discovered;
    auto res = fileSystem->scanDirectory(
        testDir.string(), 
        [&](const mnemis::core::filesystem::FileInfo& info) {
            discovered.push_back(info);
            return true;
        }
    );

    EXPECT_TRUE(res.isSuccess());
    EXPECT_EQ(discovered.size(), 0);
#endif
}

TEST_F(FileSystemTest, Cancellation) {
    createFile(testDir / "f1.mp3");
    createFile(testDir / "f2.mp3");
    createFile(testDir / "f3.mp3");

    int count = 0;
    auto res = fileSystem->scanDirectory(
        testDir.string(), 
        [&](const mnemis::core::filesystem::FileInfo& info) {
            count++;
            return false; // Cancel immediately after first file
        }
    );

    EXPECT_TRUE(res.isSuccess());
    EXPECT_EQ(count, 1);
}

TEST_F(FileSystemTest, PartialPermissionFailure) {
#if defined(__unix__) || defined(__APPLE__)
    fs::path noPermDir = testDir / "noperm";
    fs::create_directories(noPermDir);
    createFile(testDir / "ok.mp3");
    createFile(noPermDir / "hidden.mp3");

    chmod(noPermDir.c_str(), 0000);

    std::vector<mnemis::core::filesystem::FileInfo> discovered;
    std::vector<std::string> errors;

    auto res = fileSystem->scanDirectory(
        testDir.string(), 
        [&](const mnemis::core::filesystem::FileInfo& info) {
            discovered.push_back(info);
            return true;
        },
        [&](const std::string& p, const std::string& err) {
            errors.push_back(p);
        }
    );

    chmod(noPermDir.c_str(), 0777); // Restore to allow TearDown to delete

    EXPECT_TRUE(res.isSuccess());
    
    if (errors.size() > 0) {
        EXPECT_EQ(discovered.size(), 1);
        EXPECT_EQ(discovered[0].fileName, "ok.mp3");
    }
#endif
}

TEST_F(FileSystemTest, MissingPath) {
    auto res = fileSystem->scanDirectory(
        (testDir / "nonexistent").string(), 
        [](const mnemis::core::filesystem::FileInfo&) { return true; }
    );
    EXPECT_FALSE(res.isSuccess());
}

TEST_F(FileSystemTest, FileIdentityBasic) {
    createFile(testDir / "id.mp3", "12345");
    auto res = fileSystem->getFileIdentity((testDir / "id.mp3").string());
    
    ASSERT_TRUE(res.isSuccess());
    EXPECT_EQ(res.value().fileSize, 5);
    EXPECT_GT(res.value().modifiedTime, 0);
    EXPECT_FALSE(res.value().fingerprint.has_value());
}

TEST_F(FileSystemTest, DisappearingFile) {
    fs::path tempFile = testDir / "temp.mp3";
    createFile(tempFile);
    fs::remove(tempFile);

    auto res = fileSystem->getFileIdentity(tempFile.string());
    EXPECT_FALSE(res.isSuccess());
}

TEST_F(FileSystemTest, IsAccessible) {
    createFile(testDir / "acc.mp3");
    EXPECT_TRUE(fileSystem->isAccessible((testDir / "acc.mp3").string()).value());
    EXPECT_FALSE(fileSystem->isAccessible((testDir / "nonexistent.mp3").string()).value());
}