#pragma once

#include "IFileSystem.hpp"
#include "core/logging/Logger.hpp"

#include <filesystem>

namespace mnemis::filesystem {

class StdFileSystem final : public IFileSystem {
public:
    explicit StdFileSystem(
        core::Logger* logger = nullptr
    );

    core::Result<bool> exists(
        const Path& path
    ) const override;

    core::Result<bool> isDirectory(
        const Path& path
    ) const override;

    core::Result<void> createDirectories(
        const Path& path
    ) override;

    core::Result<Folder> folderInfo(
        const Path& path
    ) const override;

    core::Result<void> scanRecursive(
        const Path& root,
        const FileCallback& callback
    ) const override;

private:
    core::Logger* m_logger = nullptr;

    void logWarning(
        const std::string& message
    ) const;

    core::Result<FileInfo> makeFileInfo(
        const std::filesystem::directory_entry& entry
    ) const;
};

} // namespace mnemis::filesystem
