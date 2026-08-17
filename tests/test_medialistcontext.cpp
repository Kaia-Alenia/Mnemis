#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTimer>
#include <QEventLoop>
#include <QSignalSpy>
#include <QThreadPool>
#include <chrono>

#include "ui/controllers/MediaListContext.hpp"
#include "core/models/MediaItem.hpp"
#include "core/repositories/IMediaRepository.hpp"
#include "core/Result.hpp"

using namespace mnemis::core;
using namespace mnemis::core::models;
using namespace mnemis::core::repositories;
using namespace mnemis::ui::controllers;

class MockContextRepository : public IMediaRepository {
public:
    std::vector<std::string> orderedIds;

    void setEventBus(std::shared_ptr<events::ILibraryEventBus> eventBus) override {}

    Result<void> add(const MediaItem& item) override { return Result<void>(); }
    Result<void> update(const MediaItem& item) override { return Result<void>(); }
    Result<void> remove(const std::string& mediaId) override { return Result<void>(); }
    
    Result<std::optional<MediaItem>> getById(const std::string& mediaId) override { 
        return std::optional<MediaItem>(std::nullopt); 
    }
    
    Result<std::vector<MediaItem>> list(int page, int pageSize, const QueryOptions& options = {}) override {
        std::vector<MediaItem> result;
        int start = std::max(0, page - 1) * pageSize;
        for (int i = 0; i < pageSize && start + i < orderedIds.size(); ++i) {
            MediaItem item;
            item.mediaId = orderedIds[start + i];
            result.push_back(item);
        }
        return result;
    }
    
    Result<int> count(const QueryOptions& options = {}) override { 
        return orderedIds.size(); 
    }
    
    Result<std::optional<MediaItem>> getByCanonicalPath(const std::string& canonicalPath) override { return std::optional<MediaItem>(std::nullopt); }
    Result<void> saveBatch(const std::vector<MediaItem>& items) override { return Result<void>(); }
    Result<void> removeBatch(const std::vector<std::string>& mediaIds) override { return Result<void>(); }
};

TEST(MediaListContextTest, OpenSetsIndexAndCounts) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockContextRepository();
    for (int i = 0; i < 10; ++i) {
        repo->orderedIds.push_back("id_" + std::to_string(i));
    }

    MediaListContext context(repo);

    QSignalSpy idSpy(&context, &MediaListContext::currentMediaIdChanged);
    QSignalSpy indexSpy(&context, &MediaListContext::currentIndexChanged);

    context.open("id_5", 5);

    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(context.currentMediaId(), "id_5");
    EXPECT_EQ(context.currentIndex(), 5);
    EXPECT_EQ(context.totalCount(), 10);
    EXPECT_TRUE(context.hasNext());
    EXPECT_TRUE(context.hasPrevious());

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(MediaListContextTest, NextAndPreviousNavigateCorrectly) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockContextRepository();
    for (int i = 0; i < 3; ++i) {
        repo->orderedIds.push_back("id_" + std::to_string(i));
    }

    MediaListContext context(repo);
    context.open("id_0", 0);

    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_FALSE(context.hasPrevious());
    EXPECT_TRUE(context.hasNext());

    context.next();
    
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(context.currentIndex(), 1);
    EXPECT_EQ(context.currentMediaId(), "id_1");
    EXPECT_TRUE(context.hasPrevious());
    EXPECT_TRUE(context.hasNext());

    context.next();
    
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(context.currentIndex(), 2);
    EXPECT_EQ(context.currentMediaId(), "id_2");
    EXPECT_TRUE(context.hasPrevious());
    EXPECT_FALSE(context.hasNext());

    context.previous();

    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(context.currentIndex(), 1);
    EXPECT_EQ(context.currentMediaId(), "id_1");

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}
