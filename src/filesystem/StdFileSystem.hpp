#pragma once

#include "core/filesystem/IFileSystem.hpp"
#include "core/ILogger.hpp"
#include <memory>

namespace mnemis::filesystem {

class StdFileSystem : public core::filesystem::IFileSystem {
public:
    explicit StdFileSystem(core::ILogger* logger = nullptr);
    ~StdFileSystem() override = default;

    core::Result<void> scanDirectory(
        const std::string& directoryPath, 
        std::function<bool(const core::filesystem::FileInfo&)> callback,
        std::function<void(const std::string& path, const std::string& error)> errorCallback = nullptr,
        bool includeHidden = false) override;

    core::Result<core::filesystem::FileIdentity> getFileIdentity(const std::string& path) override;
    
    core::Result<bool> isAccessible(const std::string& path) override;

private:
    core::ILogger* m_logger;
};

} // namespace mnemis::filesystem
