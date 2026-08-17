#pragma once
#include "core/IDatabaseService.hpp"
#include "core/ILogger.hpp"
#include "core/IConfig.hpp"
#include "database/DatabaseConnection.hpp"
#include "database/repositories/SQLiteMediaRepository.hpp"
#include <memory>

namespace mnemis::database {

class SQLiteDatabaseService : public core::IDatabaseService {
public:
    SQLiteDatabaseService(core::ILogger& logger, core::IConfig& config);
    ~SQLiteDatabaseService() override;

    core::Result<void> connect(std::string_view path) override;
    void disconnect() override;
    core::repositories::IMediaRepository& getMediaRepository() override;

private:
    core::ILogger& m_logger;
    core::IConfig& m_config;
    
    DatabaseConnection m_conn;
    std::unique_ptr<repositories::SQLiteMediaRepository> m_mediaRepository;
};

} // namespace mnemis::database
