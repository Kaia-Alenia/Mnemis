#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTimer>
#include <QEventLoop>
#include <QSignalSpy>
#include <QThreadPool>
#include <chrono>

#include "ui/controllers/ViewerViewModel.hpp"
#include "ui/controllers/MediaListContext.hpp"
#include "core/models/MediaItem.hpp"
#include "core/repositories/IMediaRepository.hpp"
#include "core/Result.hpp"

using namespace mnemis::core;
using namespace mnemis::core::models;
using namespace mnemis::core::repositories;
using namespace mnemis::ui::controllers;

class MockViewerRepository : public IMediaRepository {
public:
    std::unordered_map<std::string, MediaItem> items;
    std::vector<std::string> orderedIds;
    bool updateCalled = false;
    MediaItem lastUpdatedItem;

    void setEventBus(std::shared_ptr<events::ILibraryEventBus> eventBus) override {}

    Result<void> add(const MediaItem& item) override { return Result<void>(); }
    Result<void> update(const MediaItem& item) override { 
        updateCalled = true;
        lastUpdatedItem = item;
        items[item.mediaId] = item;
        return Result<void>(); 
    }
    Result<void> remove(const std::string& mediaId) override { return Result<void>(); }
    
    Result<std::optional<MediaItem>> getById(const std::string& mediaId) override { 
        if (mediaId == "error_id") {
            return Error{-1, "Simulated error"};
        }
        auto it = items.find(mediaId);
        if (it != items.end()) {
            return std::optional<MediaItem>(it->second);
        }
        return std::optional<MediaItem>(std::nullopt); 
    }
    
    Result<std::vector<MediaItem>> list(int page, int pageSize, const QueryOptions& options = {}) override {
        std::vector<MediaItem> result;
        int start = std::max(0, page - 1) * pageSize;
        for (int i = 0; i < pageSize && start + i < orderedIds.size(); ++i) {
            result.push_back(items[orderedIds[start + i]]);
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

TEST(ViewerViewModelTest, LoadMediaItemUpdatesProperties) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    MediaItem item;
    item.mediaId = "test_id_1";
    item.canonicalPath = "/path/to/test1.jpg";
    item.fileSize = 1024;
    item.width = 1920;
    item.height = 1080;
    item.favorite = true;
    repo->items["test_id_1"] = item;
    repo->orderedIds.push_back("test_id_1");

    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    QSignalSpy stateSpy(&vm, &ViewerViewModel::imageStateChanged);
    QSignalSpy titleSpy(&vm, &ViewerViewModel::titleChanged);

    context.open("test_id_1");

    // Wait for async load
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(vm.mediaId(), "test_id_1");
    EXPECT_EQ(vm.canonicalPath(), "/path/to/test1.jpg");
    EXPECT_EQ(vm.title(), "test1.jpg");
    EXPECT_EQ(vm.fileSize(), 1024);
    EXPECT_EQ(vm.resolution(), QSize(1920, 1080));
    EXPECT_EQ(vm.isFavorite(), true);
    EXPECT_EQ(vm.imageState(), "Ready");
    EXPECT_EQ(vm.rotation(), 0);

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, ToggleFavoriteUpdatesOptimisticallyAndCallsRepo) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    MediaItem item;
    item.mediaId = "test_id_2";
    item.favorite = false;
    repo->items["test_id_2"] = item;
    repo->orderedIds.push_back("test_id_2");

    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    context.open("test_id_2");

    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_FALSE(vm.isFavorite());
    
    QSignalSpy favSpy(&vm, &ViewerViewModel::isFavoriteChanged);
    vm.toggleFavorite();
    
    // Optimistic update
    EXPECT_TRUE(vm.isFavorite());
    EXPECT_EQ(favSpy.count(), 1);

    // Wait for thread to update repo
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_TRUE(repo->updateCalled);
    EXPECT_TRUE(repo->lastUpdatedItem.favorite);

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, RotateDoesNotChangeOriginalFile) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    MediaItem item;
    item.mediaId = "test_id_3";
    repo->items["test_id_3"] = item;
    repo->orderedIds.push_back("test_id_3");

    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    context.open("test_id_3");

    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(vm.rotation(), 0);
    
    vm.rotate(90);
    EXPECT_EQ(vm.rotation(), 90);
    
    vm.rotate(90);
    EXPECT_EQ(vm.rotation(), 180);
    
    // Check that repository was not updated with rotation 
    EXPECT_FALSE(repo->updateCalled);

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, ImageStateLoadingToReady) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    MediaItem item;
    item.mediaId = "test_id_ready";
    repo->items["test_id_ready"] = item;
    repo->orderedIds.push_back("test_id_ready");

    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    QSignalSpy stateSpy(&vm, &ViewerViewModel::imageStateChanged);

    EXPECT_EQ(vm.imageState(), "Unavailable"); 
    
    context.open("test_id_ready");
    EXPECT_EQ(vm.imageState(), "Loading");

    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(vm.imageState(), "Ready");
    EXPECT_GE(stateSpy.count(), 1);

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, ImageStateError) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    QSignalSpy stateSpy(&vm, &ViewerViewModel::imageStateChanged);

    context.open("error_id");
    EXPECT_EQ(vm.imageState(), "Loading");

    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(vm.imageState(), "Error");

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, ImageStateUnavailable) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    context.open("missing_id");
    EXPECT_EQ(vm.imageState(), "Loading");

    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(vm.imageState(), "Unavailable");

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, CancelObsoleteLoads) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    MediaItem item1; item1.mediaId = "test_id_1"; repo->items["test_id_1"] = item1;
    MediaItem item2; item2.mediaId = "test_id_2"; repo->items["test_id_2"] = item2;
    repo->orderedIds = {"test_id_1", "test_id_2"};

    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    // Request first image
    context.open("test_id_1");
    // Immediately request second image before the first completes
    context.open("test_id_2");

    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    // The active image should be the second one, and first one's response should have been ignored
    EXPECT_EQ(vm.mediaId(), "test_id_2");
    EXPECT_EQ(vm.imageState(), "Ready");

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, VisualOperationSignals) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    ViewerViewModel vm(repo);

    QSignalSpy zoomSpy(&vm, &ViewerViewModel::zoomRequested);
    QSignalSpy panSpy(&vm, &ViewerViewModel::panRequested);
    QSignalSpy fitSpy(&vm, &ViewerViewModel::fitToWindowRequested);
    QSignalSpy originalSpy(&vm, &ViewerViewModel::originalSizeRequested);
    QSignalSpy fullscreenSpy(&vm, &ViewerViewModel::fullscreenRequested);

    vm.zoom(1.5);
    EXPECT_EQ(zoomSpy.count(), 1);
    EXPECT_EQ(zoomSpy.takeFirst().at(0).toReal(), 1.5);

    vm.pan(10.0, 20.0);
    EXPECT_EQ(panSpy.count(), 1);
    auto panArgs = panSpy.takeFirst();
    EXPECT_EQ(panArgs.at(0).toReal(), 10.0);
    EXPECT_EQ(panArgs.at(1).toReal(), 20.0);

    vm.fitToWindow();
    EXPECT_EQ(fitSpy.count(), 1);

    vm.originalSize();
    EXPECT_EQ(originalSpy.count(), 1);

    EXPECT_FALSE(vm.isFullscreen());
    vm.toggleFullscreen();
    EXPECT_TRUE(vm.isFullscreen());
    EXPECT_EQ(fullscreenSpy.count(), 1);
    EXPECT_EQ(fullscreenSpy.takeFirst().at(0).toBool(), true);

    vm.toggleFullscreen();
    EXPECT_FALSE(vm.isFullscreen());
    EXPECT_EQ(fullscreenSpy.count(), 1);
    EXPECT_EQ(fullscreenSpy.takeFirst().at(0).toBool(), false);

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, LoadAnimatedMediaTriggersAnimatedController) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    MediaItem item;
    item.mediaId = "animated_gif_id";
    item.canonicalPath = TEST_FIXTURE_DIR "/animated.gif";
    item.fileSize = 1024;
    item.width = 100;
    item.height = 100;
    item.mediaType = mnemis::core::models::MediaType::Gif;
    repo->items["animated_gif_id"] = item;
    repo->orderedIds.push_back("animated_gif_id");

    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    auto* animatedCtrl = vm.animatedController();
    ASSERT_NE(animatedCtrl, nullptr);

    QSignalSpy animatedSpy(animatedCtrl, &AnimatedMediaController::isAnimatedChanged);

    context.open("animated_gif_id");

    // Wait for async load in ViewerViewModel
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(vm.imageState(), "Ready");
    EXPECT_TRUE(animatedCtrl->isAnimated());
    EXPECT_TRUE(animatedCtrl->isPlaying());
    EXPECT_GE(animatedSpy.count(), 1);

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, AnimatedPauseAndRestart) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    MediaItem item;
    item.mediaId = "animated_gif_id";
    item.canonicalPath = TEST_FIXTURE_DIR "/animated.gif";
    item.fileSize = 1024;
    item.width = 100;
    item.height = 100;
    item.mediaType = mnemis::core::models::MediaType::Gif;
    repo->items["animated_gif_id"] = item;
    repo->orderedIds.push_back("animated_gif_id");

    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    auto* animatedCtrl = vm.animatedController();
    context.open("animated_gif_id");

    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_TRUE(animatedCtrl->isPlaying());

    // Pause
    animatedCtrl->pause();
    EXPECT_FALSE(animatedCtrl->isPlaying());
    EXPECT_EQ(animatedCtrl->animationState(), "Paused");

    // Restart
    animatedCtrl->restart();
    EXPECT_TRUE(animatedCtrl->isPlaying());
    EXPECT_EQ(animatedCtrl->animationState(), "Playing");

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, NavigateAwayAndBackToAnimated) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    
    MediaItem gifItem;
    gifItem.mediaId = "animated_gif_id";
    gifItem.canonicalPath = TEST_FIXTURE_DIR "/animated.gif";
    gifItem.fileSize = 1024;
    gifItem.width = 100;
    gifItem.height = 100;
    gifItem.mediaType = mnemis::core::models::MediaType::Gif;
    repo->items["animated_gif_id"] = gifItem;

    MediaItem staticItem;
    staticItem.mediaId = "static_img_id";
    staticItem.canonicalPath = "/path/to/static.jpg";
    staticItem.fileSize = 2048;
    repo->items["static_img_id"] = staticItem;

    repo->orderedIds = {"animated_gif_id", "static_img_id"};

    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    auto* animatedCtrl = vm.animatedController();

    // Open animated
    context.open("animated_gif_id");
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_TRUE(animatedCtrl->isAnimated());
    EXPECT_TRUE(animatedCtrl->isPlaying());

    // Navigate to static
    context.open("static_img_id");
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();

    // AnimatedController should have cleared (loadMedia on non-animated path)
    // Static image won't be animated
    EXPECT_EQ(vm.imageState(), "Ready");

    // Navigate back to animated
    context.open("animated_gif_id");
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_TRUE(animatedCtrl->isAnimated());
    EXPECT_TRUE(animatedCtrl->isPlaying());

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

TEST(ViewerViewModelTest, AnimatedCancellationStale) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    auto repo = new MockViewerRepository();
    
    MediaItem item1;
    item1.mediaId = "gif_1";
    item1.canonicalPath = TEST_FIXTURE_DIR "/animated.gif";
    item1.fileSize = 1024;
    repo->items["gif_1"] = item1;

    MediaItem item2;
    item2.mediaId = "gif_2";
    item2.canonicalPath = TEST_FIXTURE_DIR "/animated.gif";
    item2.fileSize = 1024;
    repo->items["gif_2"] = item2;

    repo->orderedIds = {"gif_1", "gif_2"};

    MediaListContext context(repo);
    ViewerViewModel vm(repo);
    vm.setContext(&context);

    // Rapid fire: open gif_1 then immediately gif_2
    context.open("gif_1");
    context.open("gif_2");

    QEventLoop loop;
    QTimer::singleShot(300, &loop, &QEventLoop::quit);
    loop.exec();

    // Should show gif_2, not gif_1
    EXPECT_EQ(vm.mediaId(), "gif_2");
    EXPECT_EQ(vm.imageState(), "Ready");

    QThreadPool::globalInstance()->waitForDone();
    delete repo;
}

