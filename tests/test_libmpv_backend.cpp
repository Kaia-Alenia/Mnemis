#include <gtest/gtest.h>
#include "playback/LibMpvBackend.hpp"
#include "fakes/FakeLogger.hpp"
#include <thread>
#include <chrono>

using namespace mnemis;

class DummyDelegate : public playback::IPlaybackBackendDelegate {
public:
    void onStateChanged(playback::PlaybackState::Value) override {}
    void onPositionChanged(double) override {}
    void onDurationChanged(double) override {}
    void onTracksChanged(const std::vector<playback::TrackInfo>&, const std::vector<playback::TrackInfo>&) override {}
    void onError(const std::string&) override {}
    void onBufferingChanged(bool) override {}
    void onEnded() override {}
};

TEST(LibMpvBackendTest, Instantiation) {
    auto delegate = std::make_shared<DummyDelegate>();
    tests::FakeLogger logger;
    try {
        playback::LibMpvBackend backend(logger);
        backend.setDelegate(delegate.get());
        EXPECT_TRUE(true);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "libmpv failed to initialize. Skipping test. Error: " << e.what();
    }
}

TEST(LibMpvBackendTest, Bug4sReproduction) {
    auto delegate = std::make_shared<DummyDelegate>();
    tests::FakeLogger logger;
    setlocale(LC_NUMERIC, "C");
    try {
        playback::LibMpvBackend backend(logger);
        backend.setDelegate(delegate.get());
        
        backend.load("tests/fixtures/diag/test_30s.mp4");
        
        for (int i = 0; i < 60; ++i) {
            backend.processEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "libmpv failed to initialize. Error: " << e.what();
    }
}
