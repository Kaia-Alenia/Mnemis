#include <gtest/gtest.h>
#include <QEventLoop>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <thread>
#include <chrono>

#include "infrastructure/watcher/QtFileWatcher.hpp"

using namespace mnemis::infrastructure::watcher;

class QtFileWatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(tempDir.isValid());
        rootPath = tempDir.path().toStdString();
    }

    void processEventsFor(int milliseconds) {
        QEventLoop loop;
        QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
        loop.exec();
    }

    QTemporaryDir tempDir;
    std::string rootPath;
};

TEST_F(QtFileWatcherTest, DebouncesMultipleEventsIntoOne) {
    QtFileWatcher watcher(100); // 100ms debounce
    int callbackCount = 0;
    std::string lastPath;

    watcher.setCallback([&](const std::string& path) {
        callbackCount++;
        lastPath = path;
    });

    watcher.watchDirectory(rootPath);
    processEventsFor(50); // Let initial watcher setup settle

    // Create 100 files very quickly
    for (int i = 0; i < 100; ++i) {
        QFile file(QString::fromStdString(rootPath) + QString("/file%1.txt").arg(i));
        file.open(QIODevice::WriteOnly);
        file.write("test");
        file.close();
    }

    // Wait for events and debounce
    processEventsFor(300);

    // Should only be called once (or very few times if the OS split them, but likely 1 due to 100ms debounce window covering the quick writes)
    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(lastPath, rootPath);
}

TEST_F(QtFileWatcherTest, BasicFileEventsTriggerCallback) {
    QtFileWatcher watcher(50);
    int callbackCount = 0;
    watcher.setCallback([&](const std::string& path) {
        callbackCount++;
    });

    watcher.watchDirectory(rootPath);
    processEventsFor(50); // Settle

    // Creation
    QFile file(QString::fromStdString(rootPath) + "/file.txt");
    file.open(QIODevice::WriteOnly);
    file.write("initial");
    file.close();
    
    for(int i=0; i<20 && callbackCount < 1; ++i) processEventsFor(50);
    EXPECT_GE(callbackCount, 1);
    int currentCallbacks = callbackCount;

    // Modification (some filesystems may coalesce/no-op append events)
    file.open(QIODevice::Append);
    file.write("modified");
    file.close();

    for (int i = 0; i < 20 && callbackCount < currentCallbacks + 1; ++i) {
        processEventsFor(50);
    }
    if (callbackCount < currentCallbacks + 1) {
        const QString filePath = QString::fromStdString(rootPath) + "/file.txt";
        const QString tempRenamePath = QString::fromStdString(rootPath) + "/file_renamed.txt";
        ASSERT_TRUE(QFile::rename(filePath, tempRenamePath));
        for (int i = 0; i < 20 && callbackCount < currentCallbacks + 1; ++i) {
            processEventsFor(50);
        }
        if (QFile::exists(tempRenamePath)) {
            ASSERT_TRUE(QFile::rename(tempRenamePath, filePath));
        }
    }
    EXPECT_GE(callbackCount, currentCallbacks + 1);
    currentCallbacks = callbackCount;

    // Deletion
    file.remove();
    
    for(int i=0; i<20 && callbackCount < currentCallbacks + 1; ++i) processEventsFor(50);
    EXPECT_GE(callbackCount, currentCallbacks + 1);
}

TEST_F(QtFileWatcherTest, RecursiveSubdirectoryCreation) {
    QtFileWatcher watcher(50);
    int callbackCount = 0;
    std::string reportedPath;
    
    watcher.setCallback([&](const std::string& path) {
        callbackCount++;
        reportedPath = path;
    });

    watcher.watchDirectory(rootPath);
    processEventsFor(50);
    
    // Create subdir
    QString subDirPath = QString::fromStdString(rootPath) + "/subdir";
    QDir().mkdir(subDirPath);
    
    // Wait for the subdir creation to be caught and debounced
    processEventsFor(150);
    EXPECT_GT(callbackCount, 0); // Parent dir changed
    callbackCount = 0;

    // Create a file INSIDE the new subdir
    QFile file(subDirPath + "/file.txt");
    file.open(QIODevice::WriteOnly);
    file.write("test");
    file.close();

    processEventsFor(150);
    
    // Should have caught the modification inside the new subdir
    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(reportedPath, subDirPath.toStdString());
}

TEST_F(QtFileWatcherTest, SubdirectoryDeletionRemovesWatches) {
    QtFileWatcher watcher(50);
    int callbackCount = 0;
    std::string reportedPath;

    QString subDirPath = QString::fromStdString(rootPath) + "/subdir";
    QDir().mkdir(subDirPath); // Create before watching

    watcher.setCallback([&](const std::string& path) {
        callbackCount++;
        reportedPath = path;
    });

    watcher.watchDirectory(rootPath);
    processEventsFor(50);

    // Delete subdir
    QDir().rmdir(subDirPath);

    // Wait for debounce
    for(int i=0; i<20 && callbackCount < 1; ++i) processEventsFor(50);
    EXPECT_GT(callbackCount, 0); // Root changed
    EXPECT_TRUE(reportedPath == subDirPath.toStdString() || reportedPath == rootPath);
}
