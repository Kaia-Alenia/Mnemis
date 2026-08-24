#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <memory>
#include "playback/LibMpvBackend.hpp"
#include "core/ILogger.hpp"

class ConsoleLogger : public mnemis::core::ILogger {
public:
    void log(mnemis::core::LogLevel level, std::string_view message) override {
        std::cout << message << std::endl;
    }
};

class DummyDelegate : public mnemis::playback::IPlaybackBackendDelegate {
public:
    QCoreApplication* app;
    DummyDelegate(QCoreApplication* a) : app(a) {}
    void onStateChanged(mnemis::playback::PlaybackState::Value state) override {}
    void onPositionChanged(double position) override {}
    void onDurationChanged(double duration) override {}
    void onBufferingChanged(bool isBuffering) override {}
    void onTracksChanged(const std::vector<mnemis::playback::TrackInfo>& audio,
                         const std::vector<mnemis::playback::TrackInfo>& sub) override {}
    void onEnded() override { std::cout << "onEnded called!" << std::endl; app->quit(); }
    void onError(const std::string& error) override { std::cout << "onError: " << error << std::endl; app->quit(); }
};

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ConsoleLogger logger;
    mnemis::playback::LibMpvBackend backend(logger);
    DummyDelegate delegate(&app);
    backend.setDelegate(&delegate);
    
    std::string path = argv[1];
    std::cout << "Loading: " << path << std::endl;
    backend.load(path);
    
    QTimer::singleShot(40000, &app, &QCoreApplication::quit); // 40s timeout
    return app.exec();
}
