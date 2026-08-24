#pragma once

#include "Path.hpp"

#include <string>

namespace mnemis::filesystem {

struct Folder final {
    Path path;
    Path parentPath;
    std::string name;
    bool isRoot = false;

    bool isValid() const noexcept
    {
        return !path.empty() &&
               !name.empty();
    }
};

} // namespace mnemis::filesystem
