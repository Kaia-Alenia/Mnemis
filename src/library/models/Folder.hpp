#pragma once

#include "filesystem/models/Path.hpp"
#include "library/models/LibraryRoot.hpp"

#include <cstdint>
#include <string>

namespace mnemis::library {

using FolderId = std::uint64_t;

constexpr FolderId InvalidFolderId = 0;

struct Folder final {
    FolderId id = InvalidFolderId;

    LibraryRootId rootId = InvalidLibraryRootId;
    FolderId parentId = InvalidFolderId;

    filesystem::Path path;
    std::string name;

    bool isRootFolder = false;

    bool isValid() const noexcept
    {
        return id != InvalidFolderId &&
               rootId != InvalidLibraryRootId &&
               !path.empty() &&
               !name.empty();
    }
};

} // namespace mnemis::library
