#include <gtest/gtest.h>
#include "core/indexer/MediaClassifier.hpp"

using namespace mnemis::core::indexer;
using namespace mnemis::core::models;

TEST(MediaClassifierTest, ClassifiesStandardImages) {
    auto res = MediaClassifier::classifyExtension(".jpg");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Image);
    EXPECT_EQ(res->mimeType, "image/jpeg");

    res = MediaClassifier::classifyExtension(".png");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Image);
    
    res = MediaClassifier::classifyExtension(".webp");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Image);
    
    res = MediaClassifier::classifyExtension(".gif");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Gif);
}

TEST(MediaClassifierTest, ClassifiesNewImageExtensions) {
    auto res = MediaClassifier::classifyExtension(".bmp");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Image);
    EXPECT_EQ(res->mimeType, "image/bmp");

    res = MediaClassifier::classifyExtension(".tif");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Image);
    EXPECT_EQ(res->mimeType, "image/tiff");

    res = MediaClassifier::classifyExtension(".tiff");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Image);
    EXPECT_EQ(res->mimeType, "image/tiff");
}

TEST(MediaClassifierTest, ClassifiesStandardAudio) {
    auto res = MediaClassifier::classifyExtension(".mp3");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Audio);

    res = MediaClassifier::classifyExtension(".flac");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Audio);
}

TEST(MediaClassifierTest, ClassifiesNewAudioExtensions) {
    auto res = MediaClassifier::classifyExtension(".aac");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Audio);
    EXPECT_EQ(res->mimeType, "audio/aac");
}

TEST(MediaClassifierTest, ClassifiesStandardVideo) {
    auto res = MediaClassifier::classifyExtension(".mp4");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Video);

    res = MediaClassifier::classifyExtension(".mkv");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Video);
}

TEST(MediaClassifierTest, ClassifiesNewVideoExtensions) {
    auto res = MediaClassifier::classifyExtension(".avi");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Video);
    EXPECT_EQ(res->mimeType, "video/x-msvideo");

    res = MediaClassifier::classifyExtension(".mov");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Video);
    EXPECT_EQ(res->mimeType, "video/quicktime");
}

TEST(MediaClassifierTest, HandlesCaseInsensitivity) {
    auto res = MediaClassifier::classifyExtension(".JPG");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Image);

    res = MediaClassifier::classifyExtension(".Mp4");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->mediaType, MediaType::Video);
}

TEST(MediaClassifierTest, HandlesUnknownExtensions) {
    auto res = MediaClassifier::classifyExtension(".txt");
    EXPECT_FALSE(res.has_value());

    res = MediaClassifier::classifyExtension(".doc");
    EXPECT_FALSE(res.has_value());
    
    res = MediaClassifier::classifyExtension("");
    EXPECT_FALSE(res.has_value());
}
