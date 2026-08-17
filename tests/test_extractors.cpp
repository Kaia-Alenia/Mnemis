#include <gtest/gtest.h>
#include "indexer/StbImageExtractor.hpp"
#include "indexer/TagLibExtractor.hpp"
#include "indexer/FFmpegExtractor.hpp"
#include "indexer/CompositeMetadataExtractor.hpp"
#include "core/models/MediaItem.hpp"
#include <string>
#include <filesystem>

using namespace mnemis::core;
using namespace mnemis::core::indexer;
using namespace mnemis::core::models;
using namespace mnemis::indexer;

const std::string FIXTURE_DIR = std::string(TEST_FIXTURE_DIR) + "/media/";

class ExtractorsTest : public ::testing::Test {
protected:
    StbImageExtractor stb;
    TagLibExtractor taglib;
    FFmpegExtractor ffmpeg;
    CompositeMetadataExtractor composite;
    
    ExtractorsTest() {
        composite.addExtractor(std::make_unique<StbImageExtractor>());
        composite.addExtractor(std::make_unique<TagLibExtractor>());
        composite.addExtractor(std::make_unique<FFmpegExtractor>());
    }

    void SetUp() override {
        // Ensure fixtures exist
        ASSERT_TRUE(std::filesystem::exists(FIXTURE_DIR + "test_image.png")) 
            << "Run tests/fixtures/generate_media.py first";
    }
};

TEST_F(ExtractorsTest, StbImageExtractor_ValidImages) {
    auto res_png = stb.extract(FIXTURE_DIR + "test_image.png", MediaType::Image);
    ASSERT_TRUE(res_png.isSuccess());
    EXPECT_EQ(res_png.value().width.value_or(0), 10);
    EXPECT_EQ(res_png.value().height.value_or(0), 10);
    
    auto res_jpg = stb.extract(FIXTURE_DIR + "test_image.jpg", MediaType::Image);
    ASSERT_TRUE(res_jpg.isSuccess());
    EXPECT_EQ(res_jpg.value().width.value_or(0), 10);
    
    auto res_gif = stb.extract(FIXTURE_DIR + "test_image.gif", MediaType::Image);
    ASSERT_TRUE(res_gif.isSuccess());
    EXPECT_EQ(res_gif.value().width.value_or(0), 10);
}

TEST_F(ExtractorsTest, StbImageExtractor_CorruptImage) {
    auto res = stb.extract(FIXTURE_DIR + "corrupt_image.jpg", MediaType::Image);
    EXPECT_TRUE(res.isError());
}

TEST_F(ExtractorsTest, TagLibExtractor_ValidAudio) {
    auto test_audio = [&](const std::string& filename) {
        auto res = taglib.extract(FIXTURE_DIR + filename, MediaType::Audio);
        ASSERT_TRUE(res.isSuccess()) << "Failed to extract " << filename;
        EXPECT_EQ(res.value().title.value_or(""), "Test Title");
        EXPECT_EQ(res.value().artist.value_or(""), "Test Artist");
        EXPECT_EQ(res.value().album.value_or(""), "Test Album");
        EXPECT_EQ(res.value().genre.value_or(""), "Test Genre");
        EXPECT_EQ(res.value().year.value_or(0), 2023);
        EXPECT_EQ(res.value().trackNumber.value_or(0), 1);
        EXPECT_GT(res.value().duration.value_or(0.0), 0.5);
    };
    
    test_audio("test_audio.mp3");
    test_audio("test_audio.flac");
    test_audio("test_audio.ogg");
}

TEST_F(ExtractorsTest, TagLibExtractor_Unicode) {
    auto res = taglib.extract(FIXTURE_DIR + "tëst_ünicode 🎵.mp3", MediaType::Audio);
    ASSERT_TRUE(res.isSuccess());
    EXPECT_EQ(res.value().title.value_or(""), "Títulö 🚀");
    EXPECT_EQ(res.value().artist.value_or(""), "Ärtist");
}

TEST_F(ExtractorsTest, FFmpegExtractor_ValidVideo) {
    auto test_video = [&](const std::string& filename) {
        auto res = ffmpeg.extract(FIXTURE_DIR + filename, MediaType::Video);
        ASSERT_TRUE(res.isSuccess()) << "Failed to extract " << filename;
        EXPECT_EQ(res.value().width.value_or(0), 10);
        EXPECT_EQ(res.value().height.value_or(0), 10);
        EXPECT_EQ(res.value().title.value_or(""), "Test Video Title");
        EXPECT_GT(res.value().duration.value_or(0.0), 0.5);
        EXPECT_GT(res.value().frameRate.value_or(0.0), 9.0);
    };

    test_video("test_video.mp4");
    test_video("test_video.webm");
}

TEST_F(ExtractorsTest, Extractor_Mismatch) {
    // Extracting an image disguised as mp3
    // Composite extractor tries the extractor that supports the given extension.
    // Since extension is mp3, it will use TagLib for audio. TagLib should gracefully return empty metadata or fail.
    auto res = composite.extract(FIXTURE_DIR + "mismatch.mp3", MediaType::Audio);
    if (res.isSuccess()) {
        // TagLib might just yield empty metadata if it doesn't fail parsing.
        EXPECT_EQ(res.value().duration.value_or(0.0), 0.0);
    } else {
        // Failing is also acceptable behavior.
        SUCCEED();
    }
}
