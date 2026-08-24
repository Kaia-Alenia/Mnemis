#include "Path.hpp"

#include <filesystem>
#include <utility>

namespace mnemis::filesystem {

Path::Path(std::string value)
    : m_value(std::move(value))
{
}

const std::string& Path::string() const noexcept
{
    return m_value;
}

bool Path::empty() const noexcept
{
    return m_value.empty();
}

bool Path::isAbsolute() const noexcept
{
    if (m_value.empty()) {
        return false;
    }

    return std::filesystem::path(m_value).is_absolute();
}

bool Path::isRelative() const noexcept
{
    return !isAbsolute();
}

Path Path::normalized() const
{
    if (m_value.empty()) {
        return {};
    }

    const std::filesystem::path path(m_value);

    return Path(
        path.lexically_normal().string()
    );
}

Path Path::parent() const
{
    if (m_value.empty()) {
        return {};
    }

    const std::filesystem::path path(m_value);

    return Path(
        path.parent_path().string()
    );
}

std::string Path::fileName() const
{
    if (m_value.empty()) {
        return {};
    }

    return std::filesystem::path(m_value)
        .filename()
        .string();
}

std::string Path::extension() const
{
    if (m_value.empty()) {
        return {};
    }

    return std::filesystem::path(m_value)
        .extension()
        .string();
}

bool Path::operator==(const Path& other) const noexcept
{
    return m_value == other.m_value;
}

bool Path::operator!=(const Path& other) const noexcept
{
    return !(*this == other);
}

} // namespace mnemis::filesystem
