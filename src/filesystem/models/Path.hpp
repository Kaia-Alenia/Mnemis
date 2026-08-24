#pragma once

#include <string>

namespace mnemis::filesystem {

class Path final {
public:
    Path() = default;

    explicit Path(std::string value);

    const std::string& string() const noexcept;

    bool empty() const noexcept;
    bool isAbsolute() const noexcept;
    bool isRelative() const noexcept;

    Path normalized() const;

    Path parent() const;

    std::string fileName() const;
    std::string extension() const;

    bool operator==(const Path& other) const noexcept;
    bool operator!=(const Path& other) const noexcept;

private:
    std::string m_value;
};

} // namespace mnemis::filesystem
