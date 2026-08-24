#pragma once

#include <cstdint>

namespace mnemis::media {

using MediaId = std::uint64_t;

constexpr MediaId InvalidMediaId = 0;

constexpr bool isValidMediaId(MediaId id) noexcept
{
    return id != InvalidMediaId;
}

} // namespace mnemis::media
