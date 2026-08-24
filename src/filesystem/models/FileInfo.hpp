#pragma once

#include "Path.hpp"

#include <cstdint>
#include <string>

namespace mnemis::filesystem {

struct FileInfo final {
    Path path;
    std::string fileName;
    std::string extension;

    std::int64_t fileSize = 0;
    std::int64_t modifiedTime = 0;

    bool isRegularFile = false;

    bool isValid() const noexcept
    {
        return !path.empty() &&
               isRegularFile;
    }
};

} // namespace mnemis::filesystem
