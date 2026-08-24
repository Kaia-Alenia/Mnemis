#pragma once

#include "DatabaseConnection.hpp"
#include "core/errors/Result.hpp"
#include "media/models/Media.hpp"

#include <cstddef>
#include <string>

namespace mnemis::database {

class MediaRepository final {
public:
    explicit MediaRepository(
        DatabaseConnection& database
    );

    core::Result<media::MediaId> upsert(
        const media::Media& media
    );

    core::Result<std::size_t> count();

    core::Result<std::size_t> countByType(
        media::MediaType type
    );

    core::Result<std::size_t> countByPrefix(
        const std::string& pathPrefix
    );

    core::Result<media::Media> findByPath(
        const std::string& canonicalPath
    );

private:
    DatabaseConnection& m_database;
};

} // namespace mnemis::database
