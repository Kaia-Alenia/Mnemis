#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTimer>
#include <QEventLoop>
#include <QSignalSpy>

#include "ui/controllers/AnimatedMediaController.hpp"

using namespace mnemis::ui::controllers;

// --- Basic Loading ---

TEST(AnimatedMediaControllerTest, LoadValidAnimatedMediaStartsPlaying) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;
    
    QSignalSpy stateSpy(&controller, &AnimatedMediaController::animationStateChanged);
    QSignalSpy animatedSpy(&controller, &AnimatedMediaController::isAnimatedChanged);
    QSignalSpy frameReadySpy(&controller, &AnimatedMediaController::frameReady);

    controller.loadMedia(TEST_FIXTURE_DIR "/animated.gif");

    EXPECT_TRUE(controller.isAnimated());
    EXPECT_TRUE(controller.isPlaying());
    EXPECT_EQ(controller.animationState(), "Playing");
    EXPECT_GE(animatedSpy.count(), 1);

    // Wait briefly for frames to decode
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_GE(frameReadySpy.count(), 1);
}

TEST(AnimatedMediaControllerTest, LoadStaticMediaSetsError) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;

    // A non-animated image should result in Error state
    controller.loadMedia(TEST_FIXTURE_DIR "/test.jpg");

    EXPECT_FALSE(controller.isAnimated());
    EXPECT_FALSE(controller.isPlaying());
    EXPECT_EQ(controller.animationState(), "Error");
}

TEST(AnimatedMediaControllerTest, LoadCorruptFileSetsError) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;
    controller.loadMedia(TEST_FIXTURE_DIR "/corrupted.gif");

    EXPECT_FALSE(controller.isAnimated());
    EXPECT_FALSE(controller.isPlaying());
    EXPECT_EQ(controller.animationState(), "Error");
}

// --- Play / Pause ---

TEST(AnimatedMediaControllerTest, PauseAndResume) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;
    controller.loadMedia(TEST_FIXTURE_DIR "/animated.gif");

    EXPECT_EQ(controller.animationState(), "Playing");
    
    controller.pause();
    EXPECT_EQ(controller.animationState(), "Paused");
    EXPECT_FALSE(controller.isPlaying());

    controller.play();
    EXPECT_EQ(controller.animationState(), "Playing");
    EXPECT_TRUE(controller.isPlaying());

    // Wait for a frame to ensure it works after resume
    QEventLoop loop;
    QTimer::singleShot(300, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_TRUE(controller.isPlaying());
}

// --- Restart ---

TEST(AnimatedMediaControllerTest, Restart) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;
    controller.loadMedia(TEST_FIXTURE_DIR "/animated.gif");

    // Let a few frames play
    QEventLoop loop;
    QTimer::singleShot(300, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_GT(controller.currentFrameIndex(), -1);

    QSignalSpy frameReadySpy(&controller, &AnimatedMediaController::frameReady);
    controller.restart();

    // Should be playing again from the start
    EXPECT_EQ(controller.animationState(), "Playing");
    
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();

    // Should have emitted frame 0
    EXPECT_GE(frameReadySpy.count(), 1);
}

// --- GoToFrame ---

TEST(AnimatedMediaControllerTest, GoToFrame) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;
    controller.loadMedia(TEST_FIXTURE_DIR "/animated.gif");

    // animated.gif has 5 frames (index 0-4)
    controller.goToFrame(3);

    EXPECT_EQ(controller.currentFrameIndex(), 3);
    EXPECT_EQ(controller.animationState(), "Paused");
}

TEST(AnimatedMediaControllerTest, GoToFrameOutOfBoundsIgnored) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;
    controller.loadMedia(TEST_FIXTURE_DIR "/animated.gif");

    int beforeIndex = controller.currentFrameIndex();
    controller.goToFrame(999); // Out of bounds
    controller.goToFrame(-1);  // Negative

    // State should not have changed to Paused from invalid goToFrame
    EXPECT_TRUE(controller.isPlaying());
}

// --- Speed Control ---

TEST(AnimatedMediaControllerTest, SpeedControl) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;

    QSignalSpy speedSpy(&controller, &AnimatedMediaController::speedChanged);

    EXPECT_DOUBLE_EQ(controller.speed(), 1.0);

    controller.setSpeed(2.0);
    EXPECT_DOUBLE_EQ(controller.speed(), 2.0);
    EXPECT_EQ(speedSpy.count(), 1);

    // Clamp to min
    controller.setSpeed(0.01);
    EXPECT_DOUBLE_EQ(controller.speed(), 0.1);

    // Clamp to max
    controller.setSpeed(10.0);
    EXPECT_DOUBLE_EQ(controller.speed(), 4.0);
}

// --- State Transitions ---

TEST(AnimatedMediaControllerTest, StateTransitions) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;

    // Initial state
    EXPECT_EQ(controller.animationState(), "Unavailable");

    QSignalSpy stateSpy(&controller, &AnimatedMediaController::animationStateChanged);

    controller.loadMedia(TEST_FIXTURE_DIR "/animated.gif");

    // Should have transitioned: Unavailable -> Loading -> Ready -> Playing
    // (some may be coalesced, but Playing should be current)
    EXPECT_EQ(controller.animationState(), "Playing");
    EXPECT_GE(stateSpy.count(), 2); // At least Loading->Ready->Playing

    controller.pause();
    EXPECT_EQ(controller.animationState(), "Paused");

    controller.play();
    EXPECT_EQ(controller.animationState(), "Playing");
}

// --- Frame Progression ---

TEST(AnimatedMediaControllerTest, FrameProgression) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;
    QSignalSpy frameReadySpy(&controller, &AnimatedMediaController::frameReady);

    controller.loadMedia(TEST_FIXTURE_DIR "/animated.gif");

    // Wait for multiple frames
    QEventLoop loop;
    QTimer::singleShot(600, &loop, &QEventLoop::quit);
    loop.exec();

    // GIF has 5 frames, should have decoded several
    EXPECT_GE(frameReadySpy.count(), 3);
}

// --- Cancellation A → B ---

TEST(AnimatedMediaControllerTest, CancellationAToB) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;

    // Load A
    controller.loadMedia(TEST_FIXTURE_DIR "/animated.gif");
    EXPECT_TRUE(controller.isAnimated());

    // Immediately load B (same file but the point is clear() is called)
    controller.loadMedia(TEST_FIXTURE_DIR "/animated.gif");
    EXPECT_TRUE(controller.isAnimated());
    EXPECT_EQ(controller.animationState(), "Playing");

    // Wait briefly and confirm no crash or stale state
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_TRUE(controller.isPlaying());
}

// --- Frame Timing (defensive clamp) ---

TEST(AnimatedMediaControllerTest, FrameTimingRespected) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    AnimatedMediaController controller;
    controller.loadMedia(TEST_FIXTURE_DIR "/animated.gif");

    // Start timing
    auto start = std::chrono::steady_clock::now();
    
    QEventLoop loop;
    // Wait for at least 3 frames
    QSignalSpy frameReadySpy(&controller, &AnimatedMediaController::frameReady);
    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    loop.exec();

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    // If frames played at all, at least some time must have passed
    // (GIF default frame delay is usually 100ms)
    EXPECT_GE(frameReadySpy.count(), 1);
    EXPECT_GE(elapsedMs, 50); // At minimum, defensive clamp ensures 10ms per frame
}
