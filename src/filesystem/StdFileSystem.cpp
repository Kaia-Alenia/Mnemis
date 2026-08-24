#include "StdFileSystem.hpp"

#include <chrono>
#include <filesystem>
#include <system_error>
#include <unordered_set>

namespace mnemis::filesystem {

namespace fs = std::filesystem;

namespace {

std::int64_t modifiedTime(
    const fs::directory_entry& entry
)
{
    std::error_code ec;

    const auto time =
        entry.last_write_time(ec);

    if (ec) {
        return 0;
    }

    const auto systemTime =
        std::chrono::time_point_cast<
            std::chrono::system_clock::duration
        >(
            time -
            decltype(time)::clock::now() +
            std::chrono::system_clock::now()
        );

    return std::chrono::duration_cast<
        std::chrono::seconds
    >(
        systemTime.time_since_epoch()
    ).count();
}

} // namespace

StdFileSystem::StdFileSystem(
    core::Logger* logger
)
    : m_logger(logger)
{
}

void StdFileSystem::logWarning(
    const std::string& message
) const
{
    if (m_logger != nullptr) {
        m_logger->warning(message);
    }
}

core::Result<bool> StdFileSystem::exists(
    const Path& path
) const
{
    if (path.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Filesystem",
            "Path cannot be empty"
        };
    }

    std::error_code ec;

    const bool value =
        fs::exists(
            fs::path(path.string()),
            ec
        );

    if (ec) {
        return core::Error{
            core::ErrorCode::IoError,
            "Filesystem",
            "Failed to check path existence: " +
            ec.message()
        };
    }

    return value;
}

core::Result<bool> StdFileSystem::isDirectory(
    const Path& path
) const
{
    if (path.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Filesystem",
            "Path cannot be empty"
        };
    }

    std::error_code ec;

    const bool value =
        fs::is_directory(
            fs::path(path.string()),
            ec
        );

    if (ec) {
        return core::Error{
            core::ErrorCode::IoError,
            "Filesystem",
            "Failed to inspect directory: " +
            ec.message()
        };
    }

    return value;
}

core::Result<void>
StdFileSystem::createDirectories(
    const Path& path
)
{
    if (path.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Filesystem",
            "Path cannot be empty"
        };
    }

    std::error_code ec;

    const fs::path filesystemPath(path.string());

    if (fs::exists(filesystemPath, ec)) {
        if (ec) {
            return core::Error{
                core::ErrorCode::IoError,
                "Filesystem",
                "Failed to inspect path: " +
                ec.message()
            };
        }

        if (!fs::is_directory(filesystemPath, ec)) {
            return core::Error{
                core::ErrorCode::AlreadyExists,
                "Filesystem",
                "Path exists but is not a directory: " +
                path.string()
            };
        }

        return core::Result<void>::success();
    }

    if (!fs::create_directories(filesystemPath, ec)) {
        if (ec) {
            return core::Error{
                core::ErrorCode::IoError,
                "Filesystem",
                "Failed to create directories: " +
                ec.message()
            };
        }
    }

    return core::Result<void>::success();
}

core::Result<Folder>
StdFileSystem::folderInfo(
    const Path& path
) const
{
    if (path.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Filesystem",
            "Folder path cannot be empty"
        };
    }

    std::error_code ec;

    const fs::path filesystemPath(
        path.string()
    );

    if (!fs::exists(filesystemPath, ec)) {
        if (ec) {
            return core::Error{
                core::ErrorCode::IoError,
                "Filesystem",
                "Failed to inspect folder: " +
                ec.message()
            };
        }

        return core::Error{
            core::ErrorCode::NotFound,
            "Filesystem",
            "Folder does not exist: " +
            path.string()
        };
    }

    if (!fs::is_directory(filesystemPath, ec)) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Filesystem",
            "Path is not a directory: " +
            path.string()
        };
    }

    const Path normalized =
        path.normalized();

    const Path parent =
        normalized.parent();

    Folder folder;

    folder.path = normalized;
    folder.parentPath = parent;
    folder.name =
        normalized.fileName();

    folder.isRoot =
        parent.empty() ||
        parent == normalized;

    if (folder.name.empty()) {
        folder.name = normalized.string();
    }

    return folder;
}

core::Result<FileInfo>
StdFileSystem::makeFileInfo(
    const fs::directory_entry& entry
) const
{
    std::error_code ec;

    if (!entry.is_regular_file(ec)) {
        return core::Error{
            core::ErrorCode::Unsupported,
            "Filesystem",
            "Entry is not a regular file"
        };
    }

    const fs::path filePath =
        entry.path();

    const auto fileSize =
        entry.file_size(ec);

    if (ec) {
        return core::Error{
            core::ErrorCode::IoError,
            "Filesystem",
            "Failed to read file size: " +
            ec.message()
        };
    }

    FileInfo info;

    info.path =
        Path(filePath.string()).normalized();

    info.fileName =
        filePath.filename().string();

    info.extension =
        filePath.extension().string();

    info.fileSize =
        static_cast<std::int64_t>(
            fileSize
        );

    info.modifiedTime =
        modifiedTime(entry);

    info.isRegularFile = true;

    return info;
}

core::Result<void>
StdFileSystem::scanRecursive(
    const Path& root,
    const FileCallback& callback
) const
{
    if (root.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Filesystem",
            "Root path cannot be empty"
        };
    }

    if (!callback) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Filesystem",
            "Scan callback cannot be empty"
        };
    }

    std::error_code ec;

    const fs::path rootPath =
        fs::path(root.string());

    if (!fs::exists(rootPath, ec)) {
        if (ec) {
            return core::Error{
                core::ErrorCode::IoError,
                "Filesystem",
                "Failed to inspect scan root: " +
                ec.message()
            };
        }

        return core::Error{
            core::ErrorCode::NotFound,
            "Filesystem",
            "Scan root does not exist: " +
            root.string()
        };
    }

    if (!fs::is_directory(rootPath, ec)) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Filesystem",
            "Scan root is not a directory: " +
            root.string()
        };
    }

    fs::recursive_directory_iterator iterator(
        rootPath,
        fs::directory_options::skip_permission_denied,
        ec
    );

    if (ec) {
        return core::Error{
            core::ErrorCode::PermissionDenied,
            "Filesystem",
            "Failed to open scan root: " +
            ec.message()
        };
    }

    for (const auto& entry : iterator) {
        std::error_code entryEc;

        if (entry.is_symlink(entryEc)) {
            logWarning(
                "Skipping symbolic link: " +
                entry.path().string()
            );

            iterator.disable_recursion_pending();
            continue;
        }

        if (entryEc) {
            logWarning(
                "Unable to inspect entry: " +
                entry.path().string()
            );

            continue;
        }

        if (!entry.is_regular_file(entryEc)) {
            if (entryEc) {
                logWarning(
                    "Unable to inspect entry: " +
                    entry.path().string()
                );
            }

            continue;
        }

        auto info =
            makeFileInfo(entry);

        if (info.isError()) {
            logWarning(
                "Skipping file: " +
                entry.path().string() +
                " - " +
                info.error().message()
            );

            continue;
        }

        if (!callback(info.value())) {
            break;
        }
    }

    return core::Result<void>::success();
}

} // namespace mnemis::filesystem
