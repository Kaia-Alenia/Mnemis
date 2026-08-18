#include "StdFileSystem.hpp"
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <vector>
#include <fstream>

namespace fs = std::filesystem;

namespace mnemis::filesystem {

StdFileSystem::StdFileSystem(core::ILogger* logger)
    : m_logger(logger) {}

static std::string toUtf8(const fs::path& p) {
    auto u8str = p.u8string();
    return std::string(u8str.begin(), u8str.end());
}

static fs::path fromUtf8(const std::string& s) {
    return fs::path(std::u8string(s.begin(), s.end()));
}

core::Result<void> StdFileSystem::scanDirectory(
    const std::string& directoryPath, 
    std::function<bool(const core::filesystem::FileInfo&)> callback,
    std::function<void(const std::string&, const std::string&)> errorCallback) 
{
    fs::path rootPath = fromUtf8(directoryPath);
    std::error_code ec;

    if (!fs::exists(rootPath, ec) || ec) {
        return core::Result<void>(core::Error{1, "Directory does not exist or inaccessible: " + directoryPath});
    }

    if (!fs::is_directory(rootPath, ec) || ec) {
        return core::Result<void>(core::Error{2, "Path is not a directory: " + directoryPath});
    }

    auto handleError = [&](const std::string& p, const std::string& msg) {
        if (errorCallback) {
            errorCallback(p, msg);
        } else if (m_logger) {
            m_logger->log(core::LogLevel::Error, "Filesystem error at " + p + ": " + msg);
        }
    };

    std::vector<fs::path> dirsToProcess;
    dirsToProcess.push_back(rootPath);

    while (!dirsToProcess.empty()) {
        fs::path currentDir = dirsToProcess.back();
        dirsToProcess.pop_back();

        fs::directory_iterator it(currentDir, fs::directory_options::skip_permission_denied, ec);
        if (ec) {
            handleError(toUtf8(currentDir), "Failed to open directory: " + ec.message());
            continue; // Skip this directory and continue with the rest
        }

        bool cancelled = false;
        for (const auto& entry : it) {
            std::error_code symlinkEc;
            if (entry.is_symlink(symlinkEc)) {
                handleError(toUtf8(entry.path()), "Symlink ignored");
                continue;
            }

            std::error_code statusEc;
            if (entry.is_directory(statusEc)) {
                dirsToProcess.push_back(entry.path());
                continue;
            } else if (!entry.is_regular_file(statusEc)) {
                continue;
            }

            // File size
            std::error_code sizeEc;
            uintmax_t rawSize = entry.file_size(sizeEc);
            if (sizeEc) {
                handleError(toUtf8(entry.path()), "Failed to get file size: " + sizeEc.message());
                continue;
            }

            if (rawSize > static_cast<uintmax_t>(std::numeric_limits<int64_t>::max())) {
                handleError(toUtf8(entry.path()), "File size exceeds int64_t capacity");
                continue;
            }

            int64_t fileSize = static_cast<int64_t>(rawSize);

            // Modified time
            std::error_code timeEc;
            auto ftime = entry.last_write_time(timeEc);
            int64_t modTime = 0;
            if (timeEc) {
                handleError(toUtf8(entry.path()), "Failed to get last write time: " + timeEc.message());
                continue;
            } else {
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now()
                );
                modTime = std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();
            }

            // Canonical path
            std::error_code canonicalEc;
            fs::path canonicalPath = fs::weakly_canonical(entry.path(), canonicalEc);
            if (canonicalEc) {
                handleError(toUtf8(entry.path()), "Failed to resolve canonical path: " + canonicalEc.message());
                continue;
            }

            // Extension normalization
            std::string ext = toUtf8(entry.path().extension());
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

            core::filesystem::FileInfo info;
            info.originalPath = toUtf8(entry.path());
            info.canonicalPath = toUtf8(canonicalPath);
            info.fileName = toUtf8(entry.path().filename());
            info.extension = ext;
            info.fileSize = fileSize;
            info.modifiedTime = modTime;

            if (!callback(info)) {
                cancelled = true;
                break;
            }
        }

        if (cancelled) {
            break;
        }
    }

    return core::Result<void>();
}

core::Result<core::filesystem::FileIdentity> StdFileSystem::getFileIdentity(const std::string& path) {
    fs::path p = fromUtf8(path);
    std::error_code ec;

    if (!fs::exists(p, ec) || ec) {
        return core::Result<core::filesystem::FileIdentity>(core::Error{1, "File does not exist: " + path});
    }

    if (!fs::is_regular_file(p, ec) || ec) {
        return core::Result<core::filesystem::FileIdentity>(core::Error{2, "Path is not a regular file: " + path});
    }

    fs::path canonicalPath = fs::weakly_canonical(p, ec);
    if (ec) {
        return core::Result<core::filesystem::FileIdentity>(core::Error{3, "Failed to resolve canonical path: " + ec.message()});
    }

    uintmax_t rawSize = fs::file_size(p, ec);
    if (ec) {
        return core::Result<core::filesystem::FileIdentity>(core::Error{4, "Failed to get file size: " + ec.message()});
    }
    
    if (rawSize > static_cast<uintmax_t>(std::numeric_limits<int64_t>::max())) {
        return core::Result<core::filesystem::FileIdentity>(core::Error{5, "File size exceeds int64_t capacity"});
    }

    auto ftime = fs::last_write_time(p, ec);
    if (ec) {
        return core::Result<core::filesystem::FileIdentity>(core::Error{6, "Failed to get last write time: " + ec.message()});
    }

    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now()
    );
    int64_t modTime = std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();

    core::filesystem::FileIdentity id;
    id.canonicalPath = toUtf8(canonicalPath);
    id.fileSize = static_cast<int64_t>(rawSize);
    id.modifiedTime = modTime;
    id.fingerprint = std::nullopt; // Reserved

    return core::Result<core::filesystem::FileIdentity>(id);
}

core::Result<bool> StdFileSystem::isAccessible(const std::string& path) {
    fs::path p = fromUtf8(path);
    std::error_code ec;
    
    // Check existence first
    if (!fs::exists(p, ec) || ec) {
        return core::Result<bool>(false);
    }
    
    // Check permissions
    auto status = fs::status(p, ec);
    if (ec) {
        return core::Result<bool>(false);
    }
    
    if ((status.permissions() & fs::perms::owner_read) == fs::perms::none &&
        (status.permissions() & fs::perms::group_read) == fs::perms::none &&
        (status.permissions() & fs::perms::others_read) == fs::perms::none) {
        return core::Result<bool>(false);
    }
    
    // Double check by attempting to open if it's a file
    if (fs::is_regular_file(status)) {
        std::ifstream file(p, std::ios::binary);
        if (!file.is_open()) {
            return core::Result<bool>(false);
        }
    }
    
    return core::Result<bool>(true);
}

} // namespace mnemis::filesystem
