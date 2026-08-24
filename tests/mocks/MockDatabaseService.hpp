#pragma once
#include "core/IDatabaseService.hpp"
#include <gmock/gmock.h>

namespace mnemis::tests {

class MockDatabaseService : public core::IDatabaseService {
public:
    MOCK_METHOD(core::Result<void>, connect, (std::string_view path), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(core::repositories::IMediaRepository&, getMediaRepository, (), (override));
    MOCK_METHOD(core::repositories::IPlaylistRepository&, getPlaylistRepository, (), (override));
};

} // namespace mnemis::tests
