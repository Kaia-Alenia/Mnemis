#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStandardPaths>
#include <QQmlContext>
#include <QUrl>

#include <filesystem>

#include "core/config/AppConfig.hpp"
#include "core/logging/Logger.hpp"
#include "core/storage/AppStorage.hpp"

#include "database/DatabaseManager.hpp"

#include "library/service/LibraryService.hpp"

#include "media/thumbnails/ThumbnailService.hpp"

#include "ui/controllers/LibraryController.hpp"
#include "ui/controllers/ImageViewerController.hpp"

int main(
    int argc,
    char *argv[]
)
{
    const auto config =
        mnemis::core::AppConfig::defaults();

    mnemis::core::Logger logger;

    QGuiApplication app(
        argc,
        argv
    );

    app.setApplicationName(
        QString::fromStdString(
            config.applicationName()
        )
    );

    app.setApplicationVersion(
        QString::fromStdString(
            config.applicationVersion()
        )
    );

    app.setOrganizationName(
        QString::fromStdString(
            config.organizationName()
        )
    );

    logger.info(
        "Starting Mnemis"
    );

    logger.info(
        "Version: " +
        config.applicationVersion()
    );

    // ========================================================
    // Persistent storage
    // ========================================================

    const QString qtStoragePath =
        QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation
        );

    if (qtStoragePath.isEmpty()) {
        logger.error(
            "Could not determine application data location"
        );

        return EXIT_FAILURE;
    }

    mnemis::core::AppStorage storage(
        std::filesystem::path(
            qtStoragePath.toStdString()
        )
    );

    auto storageResult =
        storage.ensureDirectories();

    if (storageResult.isError()) {
        logger.error(
            "Storage initialization failed: " +
            storageResult.error().message()
        );

        return EXIT_FAILURE;
    }

    // ========================================================
    // Database
    // ========================================================

    mnemis::database::DatabaseManager database(
        logger
    );

    auto databaseResult =
        database.open(
            storage.databasePath()
        );

    if (databaseResult.isError()) {
        logger.error(
            "Database initialization failed: " +
            databaseResult.error().message()
        );

        return EXIT_FAILURE;
    }

    logger.info(
        "Database schema version: " +
        std::to_string(
            database.schemaVersion()
        )
    );

    // ========================================================
    // Library service
    // ========================================================

    mnemis::library::LibraryService library(
        database.connection(),
        logger
    );

    mnemis::media::ThumbnailService thumbnails(
        storage.root() / "thumbnails"
    );

    auto rootCount =
        library.rootCount();

    if (rootCount.isError()) {
        logger.error(
            "Library service initialization failed: " +
            rootCount.error().message()
        );

        return EXIT_FAILURE;
    }

    logger.info(
        "Library roots: " +
        std::to_string(
            rootCount.value()
        )
    );

    // ========================================================
    // QML Controller
    // ========================================================

    mnemis::ui::LibraryController libraryController(
        library,
        thumbnails
    );

    mnemis::ui::ImageViewerController imageViewer;

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(
        QStringLiteral("libraryController"),
        &libraryController
    );

    engine.rootContext()->setContextProperty(
        QStringLiteral("imageViewer"),
        &imageViewer
    );

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [&logger] {
            logger.error(
                "Failed to create QML application"
            );

            QCoreApplication::exit(
                EXIT_FAILURE
            );
        },
        Qt::QueuedConnection
    );

    engine.load(
        QUrl(
            QStringLiteral(
                "qrc:/Main.qml"
            )
        )
    );

    if (engine.rootObjects().isEmpty()) {
        logger.error(
            "QML root object was not created"
        );

        return EXIT_FAILURE;
    }

    libraryController.refreshRoots();

    logger.info(
        "Library UI initialized successfully"
    );

    logger.info(
        "Mnemis started successfully"
    );

    return app.exec();
}
