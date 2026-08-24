#pragma once

#include "core/errors/Result.hpp"
#include "core/logging/Logger.hpp"
#include "database/MediaRepository.hpp"
#include "library/FolderRepository.hpp"
#include "library/models/LibraryRoot.hpp"
#include "filesystem/IFileSystem.hpp"
#include "media/classifier/MediaClassifier.hpp"

#include <cstddef>
#include <string>

namespace mnemis::indexer {

struct IndexStats final {
    std::size_t scannedFiles = 0;
    std::size_t indexedFiles = 0;
    std::size_t skippedFiles = 0;
};

class Indexer final {
public:
    Indexer(
        filesystem::IFileSystem& filesystem,
        media::MediaClassifier& classifier,
        database::MediaRepository& repository,
        library::FolderRepository& folderRepository,
        core::Logger& logger
    );

    core::Result<IndexStats> indexRoot(
        library::LibraryRootId rootId,
        const filesystem::Path& root
    );

private:
    filesystem::IFileSystem& m_filesystem;
    media::MediaClassifier& m_classifier;
    database::MediaRepository& m_repository;
    library::FolderRepository& m_folderRepository;
    core::Logger& m_logger;
};

} // namespace mnemis::indexer
