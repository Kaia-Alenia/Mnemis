#include "database/SQLiteDatabaseService.hpp"
#include "database/MigrationManager.hpp"


namespace mnemis::database {

SQLiteDatabaseService::SQLiteDatabaseService(core::ILogger& logger, core::IConfig& config)
    : m_logger(logger), m_config(config) {
}

SQLiteDatabaseService::~SQLiteDatabaseService() {
    disconnect();
}

core::Result<void> SQLiteDatabaseService::connect(std::string_view path) {
    if (m_conn.isOpen()) {
        return core::Result<void>(core::Error{1, "Database already connected"});
    }

    auto res = m_conn.open(path);
    if (res.isError()) {
        m_logger.log(core::LogLevel::Error, "[Database] Failed to open database: " + res.error().message);
        return res;
    }

    m_logger.log(core::LogLevel::Info, std::string("[Database] Connected to SQLite db: ") + std::string(path));

    MigrationManager migrator(m_conn, m_logger);
    auto migRes = migrator.runMigrations();
    if (migRes.isError()) {
        m_logger.log(core::LogLevel::Error, "[Database] Failed to run migrations: " + migRes.error().message);
        m_conn.close();
        return migRes;
    }

    m_mediaRepository = std::make_unique<repositories::SQLiteMediaRepository>(m_conn);
    m_playlistRepository = std::make_unique<repositories::SQLitePlaylistRepository>(m_conn, m_logger);

    return core::Result<void>();
}

void SQLiteDatabaseService::disconnect() {
    m_mediaRepository.reset();
    m_playlistRepository.reset();
    if (m_conn.isOpen()) {
        m_conn.close();
        m_logger.log(core::LogLevel::Info, "[Database] Disconnected from SQLite db");
    }
}

core::repositories::IMediaRepository& SQLiteDatabaseService::getMediaRepository() {
    if (!m_mediaRepository) {
        throw std::runtime_error("MediaRepository is not initialized. Call connect() first.");
    }
    return *m_mediaRepository;
}

core::repositories::IPlaylistRepository& SQLiteDatabaseService::getPlaylistRepository() {
    if (!m_playlistRepository) {
        throw std::runtime_error("PlaylistRepository is not initialized. Call connect() first.");
    }
    return *m_playlistRepository;
}

} // namespace mnemis::database
