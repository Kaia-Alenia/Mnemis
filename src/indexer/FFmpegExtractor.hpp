#pragma once

#include "core/indexer/IMetadataExtractor.hpp"

namespace mnemis::indexer {

class FFmpegExtractor : public core::indexer::IMetadataExtractor {
public:
    bool supports(core::models::MediaType type, const std::string& extension) const override;
    core::Result<core::indexer::Metadata> extract(const std::string& canonicalPath, core::models::MediaType type) override;
};

} // namespace mnemis::indexer
