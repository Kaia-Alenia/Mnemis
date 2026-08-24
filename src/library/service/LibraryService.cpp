#include "LibraryService.hpp"

namespace mnemis::library {

LibraryService::LibraryService(
    database::DatabaseConnection& database,
    core::Logger& logger
)
    : m_database(database)
    , m_logger(logger)
    , m_mediaRepository(database)
    , m_filesystem(&logger)
    , m_classifier()
    , m_rootRepository(database)
    , m_folderRepository(database)
    , m_mediaQuery(database)
    , m_indexer(
          m_filesystem,
          m_classifier,
          m_mediaRepository,
          m_folderRepository,
          logger
      )
{
}

core::Result<LibraryRoot>
LibraryService::validateRoot(
    LibraryRootId rootId
)
{
    if (rootId == InvalidLibraryRootId) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryService",
            "Invalid library root ID"
        };
    }

    auto result =
        m_rootRepository.findById(rootId);

    if (result.isError()) {
        return result.error();
    }

    return result.value();
}

core::Result<void>
LibraryService::validatePathInsideRoot(
    const LibraryRoot& root,
    const filesystem::Path& path
)
{
    const std::string rootPath =
        root.path.normalized().string();

    const std::string targetPath =
        path.normalized().string();

    if (targetPath == rootPath) {
        return core::Result<void>::success();
    }

    const std::string prefix =
        rootPath + "/";

    if (targetPath.rfind(prefix, 0) != 0) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryService",
            "Path is outside the selected library root"
        };
    }

    return core::Result<void>::success();
}

core::Result<LibraryRootId>
LibraryService::addRoot(
    const filesystem::Path& path,
    const std::string& name
)
{
    if (path.empty() || name.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryService",
            "Library root path and name are required"
        };
    }

    return m_rootRepository.add(
        path,
        name
    );
}

core::Result<void>
LibraryService::removeRoot(
    LibraryRootId rootId
)
{
    auto root = validateRoot(rootId);

    if (root.isError()) {
        return root.error();
    }

    return m_rootRepository.remove(rootId);
}

core::Result<void>
LibraryService::setRootEnabled(
    LibraryRootId rootId,
    bool enabled
)
{
    auto root = validateRoot(rootId);

    if (root.isError()) {
        return root.error();
    }

    return m_rootRepository.setEnabled(
        rootId,
        enabled
    );
}

core::Result<LibraryRoot>
LibraryService::getRoot(
    LibraryRootId rootId
)
{
    return validateRoot(rootId);
}

core::Result<std::vector<LibraryRoot>>
LibraryService::getRoots(
    bool includeDisabled
)
{
    return m_rootRepository.list(
        includeDisabled
    );
}

core::Result<std::size_t>
LibraryService::rootCount()
{
    return m_rootRepository.count();
}

core::Result<indexer::IndexStats>
LibraryService::indexRoot(
    LibraryRootId rootId
)
{
    auto root = validateRoot(rootId);

    if (root.isError()) {
        return root.error();
    }

    if (!root.value().enabled) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "LibraryService",
            "Cannot index a disabled library root"
        };
    }

    auto exists =
        m_filesystem.exists(
            root.value().path
        );

    if (exists.isError()) {
        return exists.error();
    }

    if (!exists.value()) {
        return core::Error{
            core::ErrorCode::NotFound,
            "LibraryService",
            "Library root does not exist"
        };
    }

    auto directory =
        m_filesystem.isDirectory(
            root.value().path
        );

    if (directory.isError()) {
        return directory.error();
    }

    if (!directory.value()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryService",
            "Library root is not a directory"
        };
    }

    return m_indexer.indexRoot(
        root.value().id,
        root.value().path
    );
}

core::Result<std::vector<media::Media>>
LibraryService::mediaInRoot(
    LibraryRootId rootId,
    bool recursive
)
{
    auto root = validateRoot(rootId);

    if (root.isError()) {
        return root.error();
    }

    return m_mediaQuery.listInRoot(
        root.value().path,
        recursive
    );
}

core::Result<std::vector<media::Media>>
LibraryService::mediaInFolder(
    LibraryRootId rootId,
    const filesystem::Path& folderPath,
    bool recursive
)
{
    auto root = validateRoot(rootId);

    if (root.isError()) {
        return root.error();
    }

    auto validation =
        validatePathInsideRoot(
            root.value(),
            folderPath
        );

    if (validation.isError()) {
        return validation.error();
    }

    return m_mediaQuery.listInFolder(
        folderPath,
        recursive
    );
}

core::Result<std::vector<Folder>>
LibraryService::foldersInRoot(
    LibraryRootId rootId
)
{
    auto root = validateRoot(rootId);

    if (root.isError()) {
        return root.error();
    }

    return m_folderRepository.listForRoot(
        rootId
    );
}

core::Result<std::vector<Folder>>
LibraryService::childFolders(
    LibraryRootId rootId,
    FolderId parentId
)
{
    auto root = validateRoot(rootId);

    if (root.isError()) {
        return root.error();
    }

    if (parentId == InvalidFolderId) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryService",
            "Invalid parent folder ID"
        };
    }

    return m_folderRepository.children(
        rootId,
        parentId
    );
}

} // namespace mnemis::library
