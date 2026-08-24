#pragma once

#include "database/DatabaseConnection.hpp"
#include "core/errors/Result.hpp"
#include "filesystem/models/Path.hpp"
#include "library/models/Folder.hpp"

#include <string>
#include <vector>

namespace mnemis::library {

class FolderRepository final {
public:
    explicit FolderRepository(
        database::DatabaseConnection& database
    );

    core::Result<FolderId> upsert(
        LibraryRootId rootId,
        FolderId parentId,
        const filesystem::Path& path,
        const std::string& name,
        bool isRootFolder
    );

    core::Result<Folder> findById(
        FolderId id
    );

    core::Result<Folder> findByPath(
        LibraryRootId rootId,
        const filesystem::Path& path
    );

    core::Result<std::vector<Folder>> children(
        LibraryRootId rootId,
        FolderId parentId
    );

    core::Result<std::vector<Folder>> listForRoot(
        LibraryRootId rootId
    );

    core::Result<std::size_t> countForRoot(
        LibraryRootId rootId
    );

private:
    database::DatabaseConnection& m_database;
};

} // namespace mnemis::library
