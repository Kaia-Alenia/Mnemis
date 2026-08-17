#pragma once

#include "core/thumbnails/IThumbnailCache.hpp"
#include <QString>
#include <QDir>
#include <mutex>
#include <unordered_map>
#include <memory>

namespace mnemis::infrastructure::thumbnails {

class DiskThumbnailCache : public core::thumbnails::IThumbnailCache {
public:
    explicit DiskThumbnailCache(const QString& cacheDir);
    ~DiskThumbnailCache() override = default;

    std::string generateKey(const core::thumbnails::ThumbnailSpec& spec) const override;
    
    std::optional<core::thumbnails::ImageBuffer> load(const std::string& key) override;
    
    bool save(const std::string& key, const core::thumbnails::ImageBuffer& image) override;
    
    void invalidate(const std::string& key) override;

private:
    QString getFilePath(const std::string& key) const;
    QString hashKey(const std::string& key) const;

    QDir m_cacheDir;
    // Note: since disk IO might be accessed from multiple threads, we don't necessarily 
    // need a mutex just for filesystem paths, but if we have any internal state (like an in-memory 
    // lru cache layer later), we'll need it.
    mutable std::mutex m_mutex;
};

} // namespace mnemis::infrastructure::thumbnails
