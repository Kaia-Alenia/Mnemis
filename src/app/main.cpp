#include <QApplication>
#include <QFileDialog>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QTranslator>
#include <QLocale>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <clocale>

// Core & Interfaces
#include "core/ApplicationContext.hpp"
#include "core/ILogger.hpp"
#include "core/IConfig.hpp"
#include "infrastructure/config/QtConfig.hpp"
#include "infrastructure/bootstrap/DefaultLibraryRoots.hpp"
#include "database/SQLiteDatabaseService.hpp"
#include "database/repositories/SQLiteMediaRepository.hpp"
#include "filesystem/StdFileSystem.hpp"
#include "infrastructure/watcher/QtFileWatcher.hpp"
#include "plugins/core/PluginManager.hpp"

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
#include "ui/controllers/PlaylistController.hpp"
#include "ui/controllers/SettingsViewModel.hpp"
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

class MnemisInputTrace final : public QObject {
public:
    bool eventFilter(QObject* watched, QEvent* event) override {
        const char* target = watched->metaObject()->className();
        const QString name = watched->objectName();
        const QString label = name.isEmpty()
            ? QString::fromLatin1(target)
            : QString::fromLatin1(target) + "#" + name;

        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick: {
            auto* mouse = static_cast<QMouseEvent*>(event);
            const char* action = event->type() == QEvent::MouseButtonPress ? "press"
                : event->type() == QEvent::MouseButtonRelease ? "release" : "double-click";
            qInfo().noquote() << "[INPUT] mouse" << action << "target=" << label
                              << "button=" << mouse->button() << "buttons=" << mouse->buttons()
                              << "position=" << mouse->position();
            break;
        }
        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            auto* key = static_cast<QKeyEvent*>(event);
            qInfo().noquote() << "[INPUT] key"
                              << (event->type() == QEvent::KeyPress ? "press" : "release")
                              << "target=" << label << "key=" << key->key()
                              << "text=" << key->text() << "modifiers=" << key->modifiers();
            break;
        }
        case QEvent::Wheel: {
            auto* wheel = static_cast<QWheelEvent*>(event);
            qInfo().noquote() << "[INPUT] wheel target=" << label
                              << "angleDelta=" << wheel->angleDelta()
                              << "position=" << wheel->position();
            break;
        }
        default:
            break;
        }
        return QObject::eventFilter(watched, event);
    }
};

class SystemPaths : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE QString picturesLocation() const {
        return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }
    Q_INVOKABLE QString moviesLocation() const {
        return QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    }
    Q_INVOKABLE QString musicLocation() const {
        return QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    }
    Q_INVOKABLE QString downloadLocation() const {
        return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }
    Q_INVOKABLE QString openFolderDialog() {
        if (!qEnvironmentVariableIsEmpty("MNEMIS_TEST_AUTO_CLICK")) {
            QTimer::singleShot(500, []() {
                for (QWidget *w : QApplication::topLevelWidgets()) {
                    if (auto *dlg = qobject_cast<QFileDialog*>(w)) {
                        qInfo() << "[TEST] Found QFileDialog, simulating selection of /media/alejandro/";
                        dlg->setDirectory("/media/alejandro/");
                        QMetaObject::invokeMethod(dlg, "accept");
                    }
                }
            });
        }
        QString dir = QFileDialog::getExistingDirectory(nullptr, "Add Library Root", QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            return "file://" + dir;
        }
        return "";
    }
};

void MnemisQtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        std::cout << "[DEBUG] [Qt] " << localMsg.constData() << " (" << file << ":" << context.line << ", " << function << ")\n";
        break;
    case QtInfoMsg:
        std::cout << "[INFO] [Qt] " << localMsg.constData() << "\n";
        break;
    case QtWarningMsg:
        std::cerr << "[WARN] [Qt] " << localMsg.constData() << " (" << file << ":" << context.line << ", " << function << ")\n";
        break;
    case QtCriticalMsg:
        std::cerr << "[CRITICAL] [Qt] " << localMsg.constData() << " (" << file << ":" << context.line << ", " << function << ")\n";
        break;
    case QtFatalMsg:
        std::cerr << "[FATAL] [Qt] " << localMsg.constData() << " (" << file << ":" << context.line << ", " << function << ")\n";
        break;
    }
}

int main(int argc, char *argv[]) {
    // Enable ultra-verbose input tracing
    qInstallMessageHandler(MnemisQtMessageHandler);

    // Force OpenGL backend to ensure libmpv can initialize GL context properly
    qputenv("QSG_RHI_BACKEND", "opengl");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QApplication app(argc, argv);
    app.installEventFilter(new MnemisInputTrace());

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "mnemis_" + QLocale(locale).name().left(2);
        if (translator.load(":/i18n/translations/" + baseName + ".qm")) {
            app.installTranslator(&translator);
            break;
        }
    }

    // Fix MPV locale requirement before QGuiApplication changes it
    std::setlocale(LC_NUMERIC, "C");

    QCoreApplication::setOrganizationName("Alenia Studios");
    QCoreApplication::setOrganizationDomain("aleniastudios.com");
    QCoreApplication::setApplicationName("Mnemis");

    qInfo() << "[DIAGNOSTICS] Ultra-verbose input tracing enabled.";

    // Set LC_NUMERIC to "C" again just in case Qt overwrites it during init
    std::setlocale(LC_NUMERIC, "C");

    Q_INIT_RESOURCE(database);

    // 1. Setup Core Infrastructure
    auto logger = std::make_unique<ConsoleLogger>();
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    std::string dbPath = dataPath.toStdString() + "/mnemis.db";
    auto config = std::make_unique<infrastructure::config::QtConfig>();
    config->setValue("database.path", dbPath);
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
        std::move(compositeExtractor),
        &context->getLogger()
    );

    // 5.5 Plugins
    auto pluginManager = std::make_shared<plugins::PluginManager>();
    pluginManager->loadPlugins(QCoreApplication::applicationDirPath() + "/plugins");
    pluginManager->notifyStartup();

    // 6. Media and Playback
    auto mpvBackend = std::make_unique<playback::LibMpvBackend>(context->getLogger());
    auto playbackEngine = std::make_unique<playback::PlaybackEngine>(std::move(mpvBackend));

    // 7. UI Controllers
    auto mediaListContext = std::make_shared<ui::controllers::MediaListContext>(repository.get());
    auto galleryModel = std::make_shared<ui::controllers::GalleryViewModel>(repository.get());
    galleryModel->setThumbnailEngine(&context->getThumbnailEngine());
    galleryModel->setMediaListContext(mediaListContext.get());
    auto viewerModel = std::make_shared<ui::controllers::ViewerViewModel>(repository.get());
    viewerModel->setContext(mediaListContext.get());
    // animatedController is owned by ViewerViewModel — do NOT create a separate one
    auto playbackController = std::make_shared<ui::controllers::PlaybackController>(std::move(playbackEngine));
    auto playlistController = std::make_shared<ui::controllers::PlaylistController>(context->getDatabase().getPlaylistRepository(), context->getLogger());

    auto settingsModel = std::make_shared<ui::controllers::SettingsViewModel>(&context->getConfig());

    // Wire settings to indexer
    settingsModel->onSettingsChanged = [indexer](const std::vector<std::string>& roots, bool includeHidden) {
        core::indexer::IndexerConfig idxConfig;
        idxConfig.includeHidden = includeHidden;
        idxConfig.batchSize = 100; // default
        indexer->setConfig(idxConfig);

        if (!roots.empty()) {
            std::thread([indexer, roots]() {
                indexer->indexRoots(roots);
            }).detach();
        }
    };

    // Wire media load to playback controller
    // GIFs/animated are handled internally by ViewerViewModel's animatedController.
    // Video and Audio are loaded and played via PlaybackController.
    // MPV will auto-play via the unpause triggered in MPV_EVENT_FILE_LOADED.
    QObject::connect(viewerModel.get(), &ui::controllers::ViewerViewModel::canonicalPathChanged, [&]() {
        QString path = viewerModel->canonicalPath();
        QString mediaId = viewerModel->mediaId();
        if (path.isEmpty() || mediaId.isEmpty()) return;

        auto result = repository->getById(mediaId.toStdString());
        if (!result.isSuccess() || !result.value().has_value()) return;

        auto type = result.value()->mediaType;
        QString uri = QStringLiteral("file://") + path;

        if (type == core::models::MediaType::Video || type == core::models::MediaType::Audio) {
            qInfo() << "[PLAYBACK] Loading" << (type == core::models::MediaType::Video ? "video" : "audio") << uri;
            playbackController->load(uri);
            // Also call play() explicitly — the state machine may reject the auto-unpause
            // from FILE_LOADED if the Loading->Playing transition isn't handled in time.
            // Calling play() after load() is safe: PlaybackEngine queues it if needed.
            playbackController->play();
        } else if (type == core::models::MediaType::Gif || type == core::models::MediaType::AnimatedWebP || type == core::models::MediaType::APNG) {
            qInfo() << "[ANIMATED] GIF/WebP/APNG will be handled by ViewerViewModel.animatedController";
        } else {
            // Static image — stop any ongoing playback
            playbackController->stop();
        }
        pluginManager->notifyMediaLoaded(mediaId);
    });

    if (!qEnvironmentVariableIsEmpty("MNEMIS_TEST_AUTO_CLICK")) {
        QTimer::singleShot(2000, [viewerModel, repository]() {
            qInfo() << "[TEST] Loading video to test MPV initialization...";
            core::models::MediaItem dummy;
            dummy.mediaId = "dummy_video";
            dummy.canonicalPath = "/tmp/sample.webm";
            dummy.mediaType = core::models::MediaType::Video;
            repository->add(dummy);
            viewerModel->open("dummy_video", -1);
        });
    }

    // 8. QML Setup
    qmlRegisterType<ui::components::AnimatedFrameItem>("mnemis.ui", 1, 0, "AnimatedFrameItem");
    qmlRegisterType<ui::components::MpvVideoItem>("mnemis.ui", 1, 0, "MpvVideoItem");

    QQmlApplicationEngine engine;

    // Provide Async Thumbnail Provider
    engine.addImageProvider("async_thumbnails", new ui::providers::AsyncThumbnailProvider(&context->getThumbnailEngine()));

    // Expose ViewModels
    SystemPaths systemPaths;
    engine.rootContext()->setContextProperty("systemPaths", &systemPaths);
    engine.rootContext()->setContextProperty("galleryModel", galleryModel.get());
    engine.rootContext()->setContextProperty("viewerModel", viewerModel.get());
    engine.rootContext()->setContextProperty("animatedController", viewerModel->animatedController());
    engine.rootContext()->setContextProperty("playbackController", playbackController.get());
    engine.rootContext()->setContextProperty("playlistController", playlistController.get());
    engine.rootContext()->setContextProperty("settingsModel", settingsModel.get());

    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    // Initial load based on settings
    std::vector<std::string> initialRoots;
    for (const auto& root : settingsModel->libraryRoots()) {
        initialRoots.push_back(root.toStdString());
    }

    if (initialRoots.empty()) {
        QStringList defaultLocations = {
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
            QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
            QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
        };

        for (const QString& loc : infrastructure::bootstrap::existingDefaultLibraryRoots(defaultLocations)) {
            initialRoots.push_back(loc.toStdString());
            settingsModel->addRoot(loc);
        }
    }

    core::indexer::IndexerConfig initialIdxConfig;
    initialIdxConfig.includeHidden = settingsModel->includeHidden();
    initialIdxConfig.batchSize = 100;
    indexer->setConfig(initialIdxConfig);

    std::thread([indexer, initialRoots]() {
        indexer->indexRoots(initialRoots);
    }).detach();

    int res = app.exec();

    // Clean shutdown
    context->shutdown();

    return res;
}

#include "main.moc"
