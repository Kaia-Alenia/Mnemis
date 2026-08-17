#pragma once

#include <gmock/gmock.h>
#include "core/thumbnails/IThumbnailEngine.hpp"

namespace mnemis::tests {

class MockThumbnailEngine : public core::thumbnails::IThumbnailEngine {
public:
    MOCK_METHOD(core::thumbnails::ThumbnailTaskId, requestThumbnail, (
        const core::thumbnails::ThumbnailSpec& spec, 
        core::thumbnails::ThumbnailPriority priority, 
        core::thumbnails::ThumbnailCallback callback
    ), (override));

    MOCK_METHOD(void, cancelRequest, (core::thumbnails::ThumbnailTaskId taskId), (override));
    MOCK_METHOD(void, clearPendingRequests, (), (override));
};

} // namespace mnemis::tests
