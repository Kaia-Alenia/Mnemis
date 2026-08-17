#include "DiskThumbnailCache.hpp"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QBuffer>
#include <QString>

namespace mnemis::infrastructure::thumbnails {

DiskThumbnailCache::DiskThumbnailCache(const QString& cacheDir)
    : m_cacheDir(cacheDir) {
    if (!m_cacheDir.exists()) {
        m_cacheDir.mkpath(".");
    }
}

std::string DiskThumbnailCache::generateKey(const core::thumbnails::ThumbnailSpec& spec) const {
    // Key must include identity, dimensions, and versions
    QString key = QString("%1|%2|%3|%4x%5|v%6")
        .arg(QString::fromStdString(spec.mediaId))
        .arg(spec.fileSize)
        .arg(spec.modifiedTime)
        .arg(spec.targetWidth)
        .arg(spec.targetHeight)
        .arg(spec.version);
    return key.toStdString();
}

QString DiskThumbnailCache::hashKey(const std::string& key) const {
    QByteArray hash = QCryptographicHash::hash(
        QByteArray::fromStdString(key), 
        QCryptographicHash::Sha256
    );
    return QString::fromLatin1(hash.toHex());
}

QString DiskThumbnailCache::getFilePath(const std::string& key) const {
    QString hashed = hashKey(key);
    // Simple 2-level directory sharding based on the first two characters of the hash
    // to avoid storing thousands of files in a single directory.
    QString subdir = hashed.left(2);
    return m_cacheDir.absoluteFilePath(subdir + "/" + hashed + ".thumb");
}

std::optional<core::thumbnails::ImageBuffer> DiskThumbnailCache::load(const std::string& key) {
    QString path = getFilePath(key);
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }

    QByteArray data = file.readAll();
    file.close();
    
    // We expect the stored data to be already encoded (e.g., JPEG/WebP) or we could just 
    // load it to QImage, but wait, if it's already encoded bytes, we might want to just return 
    // them to the frontend to decode on GPU (or UI thread natively).
    // Let's load the dimensions to fill the ImageBuffer.
    QImage img;
    if (!img.loadFromData(data)) {
        // Corrupt cache file, invalidate it
        QFile::remove(path);
        return std::nullopt;
    }

    core::thumbnails::ImageBuffer buffer;
    buffer.data = std::vector<uint8_t>(data.begin(), data.end());
    buffer.width = img.width();
    buffer.height = img.height();
    buffer.channels = img.hasAlphaChannel() ? 4 : 3;
    buffer.isEncoded = true;

    return buffer;
}

bool DiskThumbnailCache::save(const std::string& key, const core::thumbnails::ImageBuffer& image) {
    QString path = getFilePath(key);
    
    QFileInfo fileInfo(path);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    if (image.isEncoded) {
        file.write(reinterpret_cast<const char*>(image.data.data()), image.data.size());
    } else {
        // We need to encode the raw pixels to a compressed format (e.g. JPEG) to save disk space
        QImage::Format fmt = image.channels == 4 ? QImage::Format_RGBA8888 : QImage::Format_RGB888;
        // The image data might not be aligned 32-bit correctly if we just cast, 
        // but QImage takes bytesPerLine. 
        int bytesPerLine = image.width * image.channels;
        
        QImage img(image.data.data(), image.width, image.height, bytesPerLine, fmt);
        
        QByteArray outData;
        QBuffer buffer(&outData);
        buffer.open(QIODevice::WriteOnly);
        
        // Use JPEG for RGB, PNG for RGBA (thumbnails usually JPEG for performance and size)
        if (image.channels == 4) {
            img.save(&buffer, "PNG", 90);
        } else {
            img.save(&buffer, "JPG", 90);
        }
        
        file.write(outData);
    }
    
    file.close();
    return true;
}

void DiskThumbnailCache::invalidate(const std::string& key) {
    QString path = getFilePath(key);
    if (QFile::exists(path)) {
        QFile::remove(path);
    }
}

} // namespace mnemis::infrastructure::thumbnails
