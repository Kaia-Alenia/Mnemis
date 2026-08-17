#include <gtest/gtest.h>
#include "core/thumbnails/ThumbnailTypes.hpp"
#include "infrastructure/thumbnails/QtThumbnailEngine.hpp"
#include "infrastructure/thumbnails/DiskThumbnailCache.hpp"
#include <QTemporaryDir>
#include <future>
#include <chrono>
#include <thread>
#include <atomic>

using namespace mnemis::core::thumbnails;
using namespace mnemis::infrastructure::thumbnails;

class DummyDecoder : public IThumbnailDecoder {
public:
    std::atomic<bool> blockDecode{false};
    std::atomic<bool> decodeCalled{false};

    std::optional<ImageBuffer> decode(const std::string& path, const ThumbnailSpec& spec) override {
        decodeCalled = true;
        while (blockDecode) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ImageBuffer buf;
        buf.width = spec.targetWidth;
        buf.height = spec.targetHeight;
        return buf;
    }

    bool canHandle(const std::string& extension) const override {
        return true;
    }
};

class DummyCache : public IThumbnailCache {
public:
    std::string generateKey(const ThumbnailSpec& spec) const override { return "dummy_key"; }
    std::optional<ImageBuffer> load(const std::string& key) override { return std::nullopt; }
    bool save(const std::string& key, const ImageBuffer& image) override { return true; }
    void invalidate(const std::string& key) override {}
};

class QtThumbnailEngineTest : public ::testing::Test {
protected:
    std::shared_ptr<DummyCache> cache;
    std::shared_ptr<DummyDecoder> decoder;
    std::unique_ptr<QtThumbnailEngine> engine;

    void SetUp() override {
        cache = std::make_shared<DummyCache>();
        decoder = std::make_shared<DummyDecoder>();
        engine = std::make_unique<QtThumbnailEngine>(cache);
        engine->addDecoder(decoder);
    }
    
    void TearDown() override {
        decoder->blockDecode = false;
        engine.reset();
    }
};

TEST_F(QtThumbnailEngineTest, RequestSuccess) {
    std::promise<ThumbnailResult> prom;
    auto fut = prom.get_future();

    ThumbnailSpec spec{100, 100, 1, "m1", "/path/test.png", 0, 0};
    engine->requestThumbnail(spec, ThumbnailPriority::Visible, [&prom](const ThumbnailResult& res) {
        prom.set_value(res);
    });

    auto status = fut.wait_for(std::chrono::seconds(2));
    ASSERT_EQ(status, std::future_status::ready);
    
    auto res = fut.get();
    EXPECT_EQ(res.status, ThumbnailStatus::Ready);
    EXPECT_TRUE(res.image.has_value());
    EXPECT_EQ(res.image->width, 100);
}

TEST_F(QtThumbnailEngineTest, Cancellation) {
    decoder->blockDecode = true;

    std::promise<ThumbnailResult> prom;
    auto fut = prom.get_future();

    ThumbnailSpec spec{100, 100, 1, "m1", "/path/test.png", 0, 0};
    auto taskId = engine->requestThumbnail(spec, ThumbnailPriority::Visible, [&prom](const ThumbnailResult& res) {
        prom.set_value(res);
    });

    // Wait until decode is actually running (so we know it's blocked)
    // or just cancel it immediately and release block.
    // If it's queued, it will be cancelled before running.
    engine->cancelRequest(taskId);
    decoder->blockDecode = false; // let it finish if it started

    auto status = fut.wait_for(std::chrono::seconds(2));
    ASSERT_EQ(status, std::future_status::ready);
    
    auto res = fut.get();
    EXPECT_EQ(res.status, ThumbnailStatus::Cancelled);
}

TEST_F(QtThumbnailEngineTest, BackpressureRejectsBackgroundTasks) {
    decoder->blockDecode = true;

    // Fill the queue
    for (int i = 0; i < 1000; ++i) {
        ThumbnailSpec spec{100, 100, 1, "m" + std::to_string(i), "/path.png", 0, 0};
        engine->requestThumbnail(spec, ThumbnailPriority::Visible, nullptr);
    }

    // Now request a background task
    std::promise<ThumbnailResult> prom;
    auto fut = prom.get_future();
    ThumbnailSpec specBg{100, 100, 1, "mbg", "/path.png", 0, 0};
    auto bgTaskId = engine->requestThumbnail(specBg, ThumbnailPriority::Background, [&prom](const ThumbnailResult& res) {
        prom.set_value(res);
    });

    EXPECT_EQ(bgTaskId, 0); // Rejected
    
    auto status = fut.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(status, std::future_status::ready);
    auto res = fut.get();
    EXPECT_EQ(res.status, ThumbnailStatus::Cancelled);

    decoder->blockDecode = false;
}
