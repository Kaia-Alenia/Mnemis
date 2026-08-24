#pragma once

#include "filesystem/models/Path.hpp"

#include <cstdint>
#include <string>

namespace mnemis::library {

using LibraryRootId = std::uint64_t;

constexpr LibraryRootId InvalidLibraryRootId = 0;

struct LibraryRoot final {
    LibraryRootId id = InvalidLibraryRootId;

    filesystem::Path path;

    std::string name;

    bool enabled = true;

    bool isValid() const noexcept
    {
        return id != InvalidLibraryRootId &&
               !path.empty() &&
               !name.empty();
    }
};

} // namespace mnemis::library
