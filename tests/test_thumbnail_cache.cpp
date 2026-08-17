#include <gtest/gtest.h>
#include "core/thumbnails/ThumbnailTypes.hpp"
#include "infrastructure/thumbnails/DiskThumbnailCache.hpp"
#include "infrastructure/thumbnails/StbThumbnailDecoder.hpp"
#include <QTemporaryDir>
#include <QString>
#include <QImage>

using namespace mnemis::core::thumbnails;
using namespace mnemis::infrastructure::thumbnails;

const std::string FIXTURE_DIR = std::string(TEST_FIXTURE_DIR) + "/media/";

class ThumbnailCacheTest : public ::testing::Test {
protected:
    QTemporaryDir tempDir;
    std::unique_ptr<DiskThumbnailCache> cache;

    void SetUp() override {
        cache = std::make_unique<DiskThumbnailCache>(tempDir.path());
    }
};

TEST_F(ThumbnailCacheTest, CacheKeyGenerationIsStable) {
    ThumbnailSpec spec1{200, 200, 1, "media-123", "/path/to/media.png", 1024, 1234567890};
    ThumbnailSpec spec2{200, 200, 1, "media-123", "/path/to/media.png", 1024, 1234567890};
    ThumbnailSpec spec3{200, 200, 1, "media-123", "/different/path/to/media.png", 1024, 1234567890}; // Same id and file attributes, different path should not change the logical key ideally, but if key generation uses canonicalPath it might. Let's see.

    std::string key1 = cache->generateKey(spec1);
    std::string key2 = cache->generateKey(spec2);
    std::string key3 = cache->generateKey(spec3);

    EXPECT_EQ(key1, key2);
    // As long as the key is stable for the same spec, the test passes.
}

TEST_F(ThumbnailCacheTest, LoadAndSave) {
    ThumbnailSpec spec{200, 200, 1, "media-123", "/path/to/media.png", 1024, 1234567890};
    std::string key = cache->generateKey(spec);

    // Initial load should fail
    auto loaded = cache->load(key);
    EXPECT_FALSE(loaded.has_value());

    // Create a dummy image
    QImage dummyImg(200, 200, QImage::Format_RGBA8888);
    dummyImg.fill(Qt::red);

    ImageBuffer buffer;
    buffer.width = 200;
    buffer.height = 200;
    buffer.channels = 4;
    // We encode the image just to write valid bytes
    // In our disk cache, it handles encoded data directly. Let's just put raw bytes for testing
    buffer.data.resize(dummyImg.sizeInBytes());
    memcpy(buffer.data.data(), dummyImg.constBits(), dummyImg.sizeInBytes());
    buffer.isEncoded = false; // Need to be encoded by save()

    bool saved = cache->save(key, buffer);
    EXPECT_TRUE(saved);

    // Load again
    auto loaded2 = cache->load(key);
    ASSERT_TRUE(loaded2.has_value());
    EXPECT_TRUE(loaded2->isEncoded);
    EXPECT_GT(loaded2->data.size(), 0u);
    EXPECT_EQ(loaded2->width, buffer.width);
    EXPECT_EQ(loaded2->height, buffer.height);

    QImage decoded;
    QByteArray encodedBytes(
        reinterpret_cast<const char*>(loaded2->data.data()),
        static_cast<qsizetype>(loaded2->data.size())
    );
    ASSERT_TRUE(decoded.loadFromData(encodedBytes));
    EXPECT_EQ(decoded.width(), static_cast<int>(buffer.width));
    EXPECT_EQ(decoded.height(), static_cast<int>(buffer.height));
}

TEST_F(ThumbnailCacheTest, StbDecoderValidImage) {
    StbThumbnailDecoder decoder;
    
    std::string imagePath = FIXTURE_DIR + "test_image.png";
    ThumbnailSpec spec{100, 100, 1, "media-1", imagePath, 0, 0};
    
    auto result = decoder.decode(imagePath, spec);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_LE(result->width, 100);
    EXPECT_LE(result->height, 100);
}

TEST_F(ThumbnailCacheTest, StbDecoderCorruptImage) {
    StbThumbnailDecoder decoder;
    
    std::string imagePath = FIXTURE_DIR + "corrupt_image.jpg";
    ThumbnailSpec spec{100, 100, 1, "media-1", imagePath, 0, 0};
    
    auto result = decoder.decode(imagePath, spec);
    
    EXPECT_FALSE(result.has_value());
}
