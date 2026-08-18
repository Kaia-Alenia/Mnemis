#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <clocale>

// Core & Interfaces
#include "core/ApplicationContext.hpp"
#include "core/ILogger.hpp"
#include "core/IConfig.hpp"
#include "database/SQLiteDatabaseService.hpp"
#include "database/repositories/SQLiteMediaRepository.hpp"
#include "filesystem/StdFileSystem.hpp"
#include "infrastructure/watcher/QtFileWatcher.hpp"

// Indexer
#include "core/indexer/Indexer.hpp"
#include "core/indexer/MediaClassifier.hpp"
#include "indexer/CompositeMetadataExtractor.hpp"
#include "indexer/StbImageExtractor.hpp"
#include "indexer/TagLibExtractor.hpp"
#include "indexer/FFmpegExtractor.hpp"

// Thumbnails
#include "infrastructure/thumbnails/DiskThumbnailCache.hpp"
#include "infrastructure/thumbnails/QtThumbnailEngine.hpp"
#include "infrastructure/thumbnails/StbThumbnailDecoder.hpp"
#include "infrastructure/thumbnails/FFmpegThumbnailDecoder.hpp"

// Media & Playback
#include "playback/PlaybackEngine.hpp"
#include "playback/LibMpvBackend.hpp"
#include "infrastructure/media/QtAnimatedMediaDecoder.hpp"

// UI Controllers & Providers
#include "ui/controllers/GalleryViewModel.hpp"
#include "ui/controllers/MediaListContext.hpp"
#include "ui/controllers/ViewerViewModel.hpp"
#include "ui/controllers/AnimatedMediaController.hpp"
#include "ui/controllers/PlaybackController.hpp"
#include "ui/providers/AsyncThumbnailProvider.hpp"

// Components
#include "ui/components/MpvVideoItem.hpp"
#include "ui/components/AnimatedFrameItem.hpp"

using namespace mnemis;

class ConsoleLogger : public core::ILogger {
public:
    void log(core::LogLevel level, std::string_view message) override {
        const char* lvl = "INFO";
        if (level == core::LogLevel::Error) lvl = "ERROR";
        else if (level == core::LogLevel::Warning) lvl = "WARN";
        else if (level == core::LogLevel::Debug) lvl = "DEBUG";
        std::cout << "[" << lvl << "] " << message << std::endl;
    }
};

class SimpleConfig : public core::IConfig {
    std::string m_dbPath;
public:
    explicit SimpleConfig(std::string dbPath) : m_dbPath(std::move(dbPath)) {}
    std::string getValue(std::string_view key) const override {
        if (key == "database.path") return m_dbPath;
        return "";
    }
};

int main(int argc, char *argv[])
{
    // Fix MPV locale requirement before QGuiApplication changes it
    std::setlocale(LC_NUMERIC, "C");
    
    QGuiApplication app(argc, argv);
    
    // Set LC_NUMERIC to "C" again just in case Qt overwrites it during init
    std::setlocale(LC_NUMERIC, "C");
    
    Q_INIT_RESOURCE(database);

    // 1. Setup Core Infrastructure
    auto logger = std::make_unique<ConsoleLogger>();
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    std::string dbPath = dataPath.toStdString() + "/mnemis.db";
    auto config = std::make_unique<SimpleConfig>(dbPath);
    auto dbService = std::make_unique<database::SQLiteDatabaseService>(*logger, *config);

    // 2. Setup Thumbnails
    auto thumbCache = std::make_shared<infrastructure::thumbnails::DiskThumbnailCache>(QString::fromStdString(dataPath.toStdString() + "/thumbnails"));
    auto thumbEngine = std::make_unique<infrastructure::thumbnails::QtThumbnailEngine>(thumbCache);
    
    thumbEngine->addDecoder(std::make_shared<infrastructure::thumbnails::StbThumbnailDecoder>());
    thumbEngine->addDecoder(std::make_shared<infrastructure::thumbnails::FFmpegThumbnailDecoder>());

    // 3. Create Application Context
    auto context = std::make_shared<core::ApplicationContext>(
        std::move(logger),
        std::move(config),
        std::move(dbService),
        std::move(thumbEngine)
    );
    
    if (!context->initialize().isSuccess()) {
        std::cerr << "Failed to initialize ApplicationContext" << std::endl;
        return -1;
    }

    // 4. Repositories and File System
    auto repository = std::shared_ptr<core::repositories::IMediaRepository>(&context->getDatabase().getMediaRepository(), [](auto*){});
    auto fileSystem = std::make_shared<filesystem::StdFileSystem>(&context->getLogger());
    auto fileWatcher = std::make_shared<infrastructure::watcher::QtFileWatcher>();

    // 5. Indexer setup
    auto classifier = std::make_unique<core::indexer::MediaClassifier>();
    auto compositeExtractor = std::make_unique<indexer::CompositeMetadataExtractor>();
    compositeExtractor->addExtractor(std::make_shared<indexer::StbImageExtractor>());
    compositeExtractor->addExtractor(std::make_shared<indexer::TagLibExtractor>());
    compositeExtractor->addExtractor(std::make_shared<indexer::FFmpegExtractor>());
    
    auto indexer = std::make_shared<core::indexer::Indexer>(
        fileSystem,
        repository,
        std::move(compositeExtractor)
    );

    // 6. Media and Playback
    auto mpvBackend = std::make_unique<playback::LibMpvBackend>(context->getLogger());
    auto playbackEngine = std::make_unique<playback::PlaybackEngine>(std::move(mpvBackend));

    // 7. UI Controllers
    auto mediaListContext = std::make_shared<ui::controllers::MediaListContext>(repository.get());
    auto galleryModel = std::make_shared<ui::controllers::GalleryViewModel>(repository.get());
    galleryModel->setThumbnailEngine(&context->getThumbnailEngine());
    auto viewerModel = std::make_shared<ui::controllers::ViewerViewModel>(repository.get());
    viewerModel->setContext(mediaListContext.get());
    auto animatedController = std::make_shared<ui::controllers::AnimatedMediaController>();
    auto playbackController = std::make_shared<ui::controllers::PlaybackController>(std::move(playbackEngine));

    // Wire Viewer selections to specialized controllers
    QObject::connect(viewerModel.get(), &ui::controllers::ViewerViewModel::canonicalPathChanged, [&]() {
        QString uri = "file://" + viewerModel->canonicalPath();
        auto result = repository->getById(viewerModel->mediaId().toStdString());
        if (result.isSuccess() && result.value().has_value()) {
            auto type = result.value()->mediaType;
            if (type == core::models::MediaType::Gif || type == core::models::MediaType::AnimatedWebP || type == core::models::MediaType::APNG) {
                animatedController->loadMedia(uri);
                animatedController->play();
            } else if (type == core::models::MediaType::Video || type == core::models::MediaType::Audio) {
                playbackController->load(uri);
                playbackController->play();
            }
        }
    });

    // 8. QML Setup
    qmlRegisterType<ui::components::AnimatedFrameItem>("mnemis.ui", 1, 0, "AnimatedFrameItem");
    qmlRegisterType<ui::components::MpvVideoItem>("mnemis.ui", 1, 0, "MpvVideoItem");

    QQmlApplicationEngine engine;

    // Provide Async Thumbnail Provider
    engine.addImageProvider("async_thumbnails", new ui::providers::AsyncThumbnailProvider(&context->getThumbnailEngine()));

    // Expose ViewModels
    engine.rootContext()->setContextProperty("galleryModel", galleryModel.get());
    engine.rootContext()->setContextProperty("viewerModel", viewerModel.get());
    engine.rootContext()->setContextProperty("animatedController", animatedController.get());
    engine.rootContext()->setContextProperty("playbackController", playbackController.get());

    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    // Initial load
    std::string startPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation).toStdString();
    std::thread([indexer, startPath]() {
        indexer->indexDirectory(startPath);
    }).detach();

    int res = app.exec();
    
    // Clean shutdown
    context->shutdown();
    
    return res;
}
