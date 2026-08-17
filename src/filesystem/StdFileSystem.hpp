#pragma once

#include "core/filesystem/IFileSystem.hpp"
#include "core/ILogger.hpp"
#include <memory>

namespace mnemis::filesystem {

class StdFileSystem : public core::filesystem::IFileSystem {
public:
    explicit StdFileSystem(std::shared_ptr<core::ILogger> logger);
    ~StdFileSystem() override = default;

    core::Result<void> scanDirectory(
        const std::string& directoryPath, 
        std::function<bool(const core::filesystem::FileInfo&)> callback,
        std::function<void(const std::string& path, const std::string& error)> errorCallback = nullptr) override;

    core::Result<core::filesystem::FileIdentity> getFileIdentity(const std::string& path) override;
    
    core::Result<bool> isAccessible(const std::string& path) override;

private:
    std::shared_ptr<core::ILogger> m_logger;
};

} // namespace mnemis::filesystem
