#pragma once
#include "core/repositories/IMediaRepository.hpp"
#include "database/DatabaseConnection.hpp"

namespace mnemis::database::repositories {

class SQLiteMediaRepository : public core::repositories::IMediaRepository {
public:
    explicit SQLiteMediaRepository(DatabaseConnection& conn);
    ~SQLiteMediaRepository() override = default;

    void setEventBus(std::shared_ptr<core::events::ILibraryEventBus> eventBus) override;

    core::Result<void> add(const core::models::MediaItem& item) override;
    core::Result<void> update(const core::models::MediaItem& item) override;
    core::Result<void> remove(const std::string& mediaId) override;
    core::Result<std::optional<core::models::MediaItem>> getById(const std::string& mediaId) override;
    core::Result<std::vector<core::models::MediaItem>> list(int page, int pageSize, const core::repositories::QueryOptions& options = {}) override;
    core::Result<int> count(const core::repositories::QueryOptions& options = {}) override;
    
    core::Result<std::optional<core::models::MediaItem>> getByCanonicalPath(const std::string& canonicalPath) override;
    core::Result<void> saveBatch(const std::vector<core::models::MediaItem>& items) override;
    core::Result<void> removeBatch(const std::vector<std::string>& mediaIds) override;

private:
    DatabaseConnection& m_conn;
    std::shared_ptr<core::events::ILibraryEventBus> m_eventBus;
};

} // namespace mnemis::database::repositories
