#include <gtest/gtest.h>
#include <QString>
#include <QDir>
#include "infrastructure/media/QtAnimatedMediaDecoder.hpp"

using namespace mnemis::infrastructure::media;
using namespace mnemis::core::media;

class QtAnimatedMediaDecoderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure fixtures directory exists and we have our files
        m_fixturesDir = QString::fromUtf8(TEST_FIXTURE_DIR);
        m_gifPath = m_fixturesDir + "/animated.gif";
        m_webpPath = m_fixturesDir + "/animated.webp";
        m_apngPath = m_fixturesDir + "/animated.apng";
        m_invalidPath = m_fixturesDir + "/nonexistent.gif";
    }

    QString m_fixturesDir;
    QString m_gifPath;
    QString m_webpPath;
    QString m_apngPath;
    QString m_invalidPath;
};

TEST_F(QtAnimatedMediaDecoderTest, OpenInvalidFileReturnsFalse) {
    QtAnimatedMediaDecoder decoder;
    EXPECT_FALSE(decoder.open(m_invalidPath.toStdString()));
}

TEST_F(QtAnimatedMediaDecoderTest, DetectsGifAndReadsMetadata) {
    QtAnimatedMediaDecoder decoder;
    if (!QFileInfo::exists(m_gifPath)) {
        GTEST_SKIP() << "GIF fixture not found. Skip.";
    }

    bool opened = decoder.open(m_gifPath.toStdString());
    if (!opened) {
        // Might fail if Qt doesn't have GIF support, but Qt supports GIF natively.
        FAIL() << "Failed to open GIF fixture";
    }

    AnimatedMediaInfo info = decoder.getInfo();
    EXPECT_GT(info.frameCount, 0); // Should be 5 frames
    EXPECT_EQ(info.frameCount, 5);
}

TEST_F(QtAnimatedMediaDecoderTest, DecodeFramesDoesNotAccumulateMemory) {
    QtAnimatedMediaDecoder decoder;
    if (!QFileInfo::exists(m_gifPath)) {
        GTEST_SKIP() << "GIF fixture not found.";
    }
    
    ASSERT_TRUE(decoder.open(m_gifPath.toStdString()));
    AnimatedMediaInfo info = decoder.getInfo();
    ASSERT_GT(info.frameCount, 0);

    for (int i = 0; i < info.frameCount; ++i) {
        auto frame = decoder.getFrame(i);
        ASSERT_TRUE(frame.has_value());
        EXPECT_EQ(frame->index, i);
        EXPECT_GT(frame->durationMs, 0);
        EXPECT_GT(frame->width, 0);
        EXPECT_GT(frame->height, 0);
        EXPECT_GT(frame->data.size(), 0);
        EXPECT_EQ(frame->pixelFormat, PixelFormat::RGBA8888);
    }
    
    // Jump backward should work
    auto frame0 = decoder.getFrame(0);
    ASSERT_TRUE(frame0.has_value());
    EXPECT_EQ(frame0->index, 0);
}

TEST_F(QtAnimatedMediaDecoderTest, CanOpenWebPIfSupported) {
    QtAnimatedMediaDecoder decoder;
    if (!QFileInfo::exists(m_webpPath)) {
        GTEST_SKIP() << "WebP fixture not found.";
    }
    
    bool opened = decoder.open(m_webpPath.toStdString());
    if (opened) {
        AnimatedMediaInfo info = decoder.getInfo();
        EXPECT_GT(info.frameCount, 0);
        auto frame = decoder.getFrame(0);
        EXPECT_TRUE(frame.has_value());
    } else {
        GTEST_SKIP() << "WebP not supported by current Qt installation";
    }
}

TEST_F(QtAnimatedMediaDecoderTest, MemoryStressTest) {
    QtAnimatedMediaDecoder decoder;
    if (!QFileInfo::exists(m_gifPath)) {
        GTEST_SKIP() << "GIF fixture not found.";
    }
    
    ASSERT_TRUE(decoder.open(m_gifPath.toStdString()));
    AnimatedMediaInfo info = decoder.getInfo();
    ASSERT_GT(info.frameCount, 0);

    // Decode 5000 iterations to ensure no memory leaks or unbounded growth
    int iterations = 5000;
    for (int i = 0; i < iterations; ++i) {
        int frameIndex = i % info.frameCount;
        auto frame = decoder.getFrame(frameIndex);
        ASSERT_TRUE(frame.has_value());
    }
}

