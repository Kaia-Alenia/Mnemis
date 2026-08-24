#pragma once

#include "models/FileInfo.hpp"
#include "models/Folder.hpp"
#include "models/Path.hpp"
#include "core/errors/Result.hpp"

#include <functional>
#include <vector>

namespace mnemis::filesystem {

class IFileSystem {
public:
    using FileCallback =
        std::function<bool(const FileInfo&)>;

    virtual ~IFileSystem() = default;

    virtual core::Result<bool> exists(
        const Path& path
    ) const = 0;

    virtual core::Result<bool> isDirectory(
        const Path& path
    ) const = 0;

    virtual core::Result<void> createDirectories(
        const Path& path
    ) = 0;

    virtual core::Result<Folder> folderInfo(
        const Path& path
    ) const = 0;

    virtual core::Result<void> scanRecursive(
        const Path& root,
        const FileCallback& callback
    ) const = 0;
};

} // namespace mnemis::filesystem
