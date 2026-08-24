#include "Indexer.hpp"

#include "media/models/Media.hpp"
#include <filesystem>

namespace mnemis::indexer {

Indexer::Indexer(
    filesystem::IFileSystem& filesystem,
    media::MediaClassifier& classifier,
    database::MediaRepository& repository,
    library::FolderRepository& folderRepository,
    core::Logger& logger
)
    : m_filesystem(filesystem)
    , m_classifier(classifier)
    , m_repository(repository)
    , m_folderRepository(folderRepository)
    , m_logger(logger)
{
}

core::Result<IndexStats>
Indexer::indexRoot(
    library::LibraryRootId rootId,
    const filesystem::Path& root
)
{
    if (rootId == library::InvalidLibraryRootId) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Indexer",
            "Library root ID cannot be invalid"
        };
    }

    if (root.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Indexer",
            "Index root cannot be empty"
        };
    }

    IndexStats stats;

    auto rootExists =
        m_filesystem.exists(root);

    if (rootExists.isError()) {
        return rootExists.error();
    }

    if (!rootExists.value()) {
        return core::Error{
            core::ErrorCode::NotFound,
            "Indexer",
            "Index root does not exist"
        };
    }

    auto rootDirectory =
        m_filesystem.isDirectory(root);

    if (rootDirectory.isError()) {
        return rootDirectory.error();
    }

    if (!rootDirectory.value()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Indexer",
            "Index root is not a directory"
        };
    }

    m_logger.info(
        "Indexing root: " +
        root.string()
    );

    auto scanResult =
        m_filesystem.scanRecursive(
            root,
            [&](const filesystem::FileInfo& file) {

                ++stats.scannedFiles;

                const auto normalizedRoot =
                    root.normalized();

                const auto normalizedFile =
                    file.path.normalized();

                std::filesystem::path relative =
                    std::filesystem::relative(
                        std::filesystem::path(
                            normalizedFile.string()
                        ),
                        std::filesystem::path(
                            normalizedRoot.string()
                        )
                    );

                if (relative.empty()) {
                    return true;
                }

                const std::filesystem::path parent =
                    relative.parent_path();

                library::FolderId parentId =
                    library::InvalidFolderId;

                const std::string rootFolderName =
                    normalizedRoot.fileName().empty()
                        ? normalizedRoot.string()
                        : normalizedRoot.fileName();

                auto rootFolderResult =
                    m_folderRepository.upsert(
                        rootId,
                        library::InvalidFolderId,
                        normalizedRoot,
                        rootFolderName,
                        true
                    );

                if (rootFolderResult.isError()) {
                    m_logger.error(
                        "Failed to create root folder: " +
                        rootFolderResult.error().message()
                    );

                    return false;
                }

                parentId =
                    rootFolderResult.value();

                std::filesystem::path currentPath(
                    normalizedRoot.string()
                );

                std::filesystem::path remaining =
                    parent;

                for (const auto& component : remaining) {

                    if (
                        component.empty() ||
                        component == "."
                    ) {
                        continue;
                    }

                    currentPath /= component;

                    auto folderResult =
                        m_folderRepository.upsert(
                            rootId,
                            parentId,
                            filesystem::Path(
                                currentPath.string()
                            ),
                            component.string(),
                            false
                        );

                    if (folderResult.isError()) {
                        m_logger.error(
                            "Failed to create folder: " +
                            currentPath.string() +
                            " - " +
                            folderResult.error().message()
                        );

                        return false;
                    }

                    parentId =
                        folderResult.value();
                }

                auto classification =
                    m_classifier.classifyPath(
                        file.path.string()
                    );

                if (classification.isError()) {
                    ++stats.skippedFiles;

                    m_logger.debug(
                        "Skipping unsupported file: " +
                        file.path.string()
                    );

                    return true;
                }

                media::Media media;

                media.type =
                    classification.value();

                media.canonicalPath =
                    file.path.normalized().string();

                media.fileName =
                    file.fileName;

                media.extension =
                    file.extension;

                media.fileSize =
                    file.fileSize;

                media.modifiedTime =
                    file.modifiedTime;

                auto upsertResult =
                    m_repository.upsert(media);

                if (upsertResult.isError()) {
                    m_logger.error(
                        "Failed to index file: " +
                        file.path.string() +
                        " - " +
                        upsertResult.error().message()
                    );

                    return false;
                }

                ++stats.indexedFiles;

                return true;
            }
        );

    if (scanResult.isError()) {
        return scanResult.error();
    }

    m_logger.info(
        "Index complete. Scanned: " +
        std::to_string(stats.scannedFiles) +
        ", indexed: " +
        std::to_string(stats.indexedFiles) +
        ", skipped: " +
        std::to_string(stats.skippedFiles)
    );

    return stats;
}

} // namespace mnemis::indexer
