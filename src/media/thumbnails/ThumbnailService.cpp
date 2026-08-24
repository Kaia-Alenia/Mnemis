#include "ThumbnailService.hpp"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QSaveFile>
#include <QString>

#include <filesystem>
#include <string>

namespace mnemis::media {

ThumbnailService::ThumbnailService(
    std::filesystem::path cacheDirectory
)
    : m_cacheDirectory(std::move(cacheDirectory))
{
}

const std::filesystem::path&
ThumbnailService::cacheDirectory() const noexcept
{
    return m_cacheDirectory;
}

core::Result<void>
ThumbnailService::ensureCacheDirectory() const
{
    if (m_cacheDirectory.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "ThumbnailService",
            "Thumbnail cache directory cannot be empty"
        };
    }

    std::error_code error;

    std::filesystem::create_directories(
        m_cacheDirectory,
        error
    );

    if (error) {
        return core::Error{
            core::ErrorCode::IoError,
            "ThumbnailService",
            "Failed to create thumbnail cache directory: " +
            error.message()
        };
    }

    return core::Result<void>::success();
}

std::filesystem::path
ThumbnailService::cachePathFor(
    const Media& media,
    int maxWidth,
    int maxHeight
) const
{
    const std::string cacheKey =
        media.canonicalPath +
        "|" +
        std::to_string(media.modifiedTime) +
        "|" +
        std::to_string(maxWidth) +
        "x" +
        std::to_string(maxHeight);

    const QByteArray hash =
        QCryptographicHash::hash(
            QByteArray::fromStdString(
                cacheKey
            ),
            QCryptographicHash::Sha256
        );

    const QString fileName =
        QString::fromLatin1(
            hash.toHex()
        ) +
        QStringLiteral(".png");

    return m_cacheDirectory /
        fileName.toStdString();
}

core::Result<std::filesystem::path>
ThumbnailService::thumbnailFor(
    const Media& media,
    int maxWidth,
    int maxHeight
)
{
    if (!media.isValid()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "ThumbnailService",
            "Invalid media object"
        };
    }

    if (maxWidth <= 0 ||
        maxHeight <= 0) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "ThumbnailService",
            "Thumbnail dimensions must be positive"
        };
    }

    if (media.type != MediaType::Image) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "ThumbnailService",
            "Thumbnail generation currently supports images only"
        };
    }

    auto cacheDirectoryResult =
        ensureCacheDirectory();

    if (cacheDirectoryResult.isError()) {
        return cacheDirectoryResult.error();
    }

    const auto cachePath =
        cachePathFor(
            media,
            maxWidth,
            maxHeight
        );

    if (std::filesystem::exists(cachePath)) {
        return cachePath;
    }

    return generateImageThumbnail(
        media,
        maxWidth,
        maxHeight
    );
}

core::Result<std::filesystem::path>
ThumbnailService::generateImageThumbnail(
    const Media& media,
    int maxWidth,
    int maxHeight
)
{
    QImageReader reader(
        QString::fromStdString(
            media.canonicalPath
        )
    );

    reader.setAutoTransform(
        true
    );

    QImage image =
        reader.read();

    if (image.isNull()) {
        return core::Error{
            core::ErrorCode::IoError,
            "ThumbnailService",
            "Failed to decode image: " +
            media.canonicalPath
        };
    }

    const QImage::Format preferredFormat =
        image.hasAlphaChannel()
            ? QImage::Format_RGBA8888
            : QImage::Format_RGB888;

    image =
        image.convertToFormat(
            preferredFormat
        );

    // FastTransformation is intentional:
    // thumbnails must remain crisp for pixel-art assets.
    QImage thumbnail =
        image.scaled(
            maxWidth,
            maxHeight,
            Qt::KeepAspectRatio,
            Qt::FastTransformation
        );

    if (thumbnail.isNull()) {
        return core::Error{
            core::ErrorCode::IoError,
            "ThumbnailService",
            "Failed to scale image: " +
            media.canonicalPath
        };
    }

    const auto cachePath =
        cachePathFor(
            media,
            maxWidth,
            maxHeight
        );

    QSaveFile output(
        QString::fromStdString(
            cachePath.string()
        )
    );

    if (!output.open(
            QIODevice::WriteOnly
        )) {

        return core::Error{
            core::ErrorCode::IoError,
            "ThumbnailService",
            "Failed to open thumbnail cache file: " +
            cachePath.string()
        };
    }

    if (!thumbnail.save(
            &output,
            "PNG"
        )) {

        output.cancelWriting();

        return core::Error{
            core::ErrorCode::IoError,
            "ThumbnailService",
            "Failed to encode thumbnail: " +
            cachePath.string()
        };
    }

    if (!output.commit()) {
        return core::Error{
            core::ErrorCode::IoError,
            "ThumbnailService",
            "Failed to commit thumbnail cache file: " +
            cachePath.string()
        };
    }

    return cachePath;
}

core::Result<void>
ThumbnailService::clearCache()
{
    if (m_cacheDirectory.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "ThumbnailService",
            "Thumbnail cache directory cannot be empty"
        };
    }

    if (!std::filesystem::exists(
            m_cacheDirectory
        )) {

        return core::Result<void>::success();
    }

    std::error_code error;

    std::filesystem::remove_all(
        m_cacheDirectory,
        error
    );

    if (error) {
        return core::Error{
            core::ErrorCode::IoError,
            "ThumbnailService",
            "Failed to clear thumbnail cache: " +
            error.message()
        };
    }

    return core::Result<void>::success();
}

} // namespace mnemis::media
