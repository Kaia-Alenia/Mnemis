#pragma once

#include "MediaSort.hpp"

#include "core/errors/Result.hpp"
#include "database/DatabaseConnection.hpp"
#include "filesystem/models/Path.hpp"
#include "media/models/Media.hpp"
#include "media/models/MediaType.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace mnemis::library {

struct MediaQueryOptions final {
    std::optional<filesystem::Path> scopePath;

    bool recursive = true;

    std::string searchText;

    std::optional<media::MediaType> mediaType;

    MediaSortField sortField =
        MediaSortField::Name;

    SortDirection direction =
        SortDirection::Ascending;

    std::size_t limit = 0;

    std::size_t offset = 0;
};

class MediaQuery final {
public:
    explicit MediaQuery(
        database::DatabaseConnection& database
    );

    core::Result<std::vector<media::Media>> execute(
        const MediaQueryOptions& options
    );

    core::Result<std::size_t> count(
        const MediaQueryOptions& options
    );

    core::Result<std::vector<media::Media>> listInRoot(
        const filesystem::Path& rootPath,
        bool recursive = true
    );

    core::Result<std::vector<media::Media>> listInFolder(
        const filesystem::Path& folderPath,
        bool recursive = true
    );

private:
    database::DatabaseConnection& m_database;

    static const char* sortColumn(
        MediaSortField field
    ) noexcept;

    static std::string escapeLike(
        const std::string& value
    );

    core::Result<std::vector<media::Media>>
    executeInternal(
        const MediaQueryOptions& options,
        bool countOnly
    );
};

} // namespace mnemis::library
