#include <gtest/gtest.h>
#include "ui/controllers/PlaybackController.hpp"
#include "fakes/FakePlaybackBackend.hpp"

using namespace mnemis;

TEST(PlaybackControllerTest, StatePropertyAndSignals) {
    auto backendRaw = new tests::fakes::FakePlaybackBackend();
    auto engine = std::make_unique<playback::PlaybackEngine>(
        std::unique_ptr<playback::IPlaybackBackend>(backendRaw)
    );
    ui::controllers::PlaybackController controller(std::move(engine));

    bool stateChanged = false;
    QObject::connect(&controller, &ui::controllers::PlaybackController::stateChanged, [&]() {
        stateChanged = true;
    });

    EXPECT_EQ(controller.state(), "Idle");
    backendRaw->simulateStateChange(playback::PlaybackState::Loading);
    backendRaw->simulateStateChange(playback::PlaybackState::Ready);
    EXPECT_TRUE(stateChanged);
    EXPECT_EQ(controller.state(), "Ready");
}

TEST(PlaybackControllerTest, TracksMapping) {
    auto backendRaw = new tests::fakes::FakePlaybackBackend();
    auto engine = std::make_unique<playback::PlaybackEngine>(
        std::unique_ptr<playback::IPlaybackBackend>(backendRaw)
    );
    ui::controllers::PlaybackController controller(std::move(engine));

    bool tracksChanged = false;
    QObject::connect(&controller, &ui::controllers::PlaybackController::tracksChanged, [&]() {
        tracksChanged = true;
    });

    std::vector<playback::TrackInfo> audio = {
        {
            {"id", 1},
            {"type", "audio"},
            {"title", "Stereo"},
            {"lang", "en"},
            {"codec", "aac"},
            {"selected", true}
        }
    };
    std::vector<playback::TrackInfo> subs = {};

    backendRaw->simulateTracksChanged(audio, subs);
    EXPECT_TRUE(tracksChanged);

    auto qAudio = controller.audioTracks();
    EXPECT_EQ(qAudio.size(), 1);
    auto map = qAudio.first().toMap();
    EXPECT_EQ(map["id"].toInt(), 1);
    EXPECT_EQ(map["title"].toString(), "Stereo");
}
