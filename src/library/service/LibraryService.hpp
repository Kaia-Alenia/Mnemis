#pragma once

#include "core/errors/Result.hpp"
#include "core/logging/Logger.hpp"

#include "database/DatabaseConnection.hpp"
#include "database/MediaRepository.hpp"

#include "filesystem/models/Path.hpp"
#include "filesystem/StdFileSystem.hpp"

#include "indexer/Indexer.hpp"

#include "library/FolderRepository.hpp"
#include "library/LibraryRootRepository.hpp"
#include "library/models/Folder.hpp"
#include "library/models/LibraryRoot.hpp"
#include "library/query/MediaQuery.hpp"

#include "media/classifier/MediaClassifier.hpp"
#include "media/models/Media.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace mnemis::library {

class LibraryService final {
public:
    LibraryService(
        database::DatabaseConnection& database,
        core::Logger& logger
    );

    core::Result<LibraryRootId> addRoot(
        const filesystem::Path& path,
        const std::string& name
    );

    core::Result<void> removeRoot(
        LibraryRootId rootId
    );

    core::Result<void> setRootEnabled(
        LibraryRootId rootId,
        bool enabled
    );

    core::Result<LibraryRoot> getRoot(
        LibraryRootId rootId
    );

    core::Result<std::vector<LibraryRoot>> getRoots(
        bool includeDisabled = true
    );

    core::Result<std::size_t> rootCount();

    core::Result<indexer::IndexStats> indexRoot(
        LibraryRootId rootId
    );

    core::Result<std::vector<media::Media>> mediaInRoot(
        LibraryRootId rootId,
        bool recursive = true
    );

    core::Result<std::vector<media::Media>> mediaInFolder(
        LibraryRootId rootId,
        const filesystem::Path& folderPath,
        bool recursive = true
    );

    core::Result<std::vector<Folder>> foldersInRoot(
        LibraryRootId rootId
    );

    core::Result<std::vector<Folder>> childFolders(
        LibraryRootId rootId,
        FolderId parentId
    );

private:
    database::DatabaseConnection& m_database;
    core::Logger& m_logger;

    database::MediaRepository m_mediaRepository;
    filesystem::StdFileSystem m_filesystem;
    media::MediaClassifier m_classifier;

    LibraryRootRepository m_rootRepository;
    FolderRepository m_folderRepository;
    MediaQuery m_mediaQuery;
    indexer::Indexer m_indexer;

    core::Result<LibraryRoot> validateRoot(
        LibraryRootId rootId
    );

    core::Result<void> validatePathInsideRoot(
        const LibraryRoot& root,
        const filesystem::Path& path
    );
};

} // namespace mnemis::library
