#pragma once

#include "core/indexer/IMetadataExtractor.hpp"
#include <vector>
#include <memory>

namespace mnemis::indexer {

class CompositeMetadataExtractor : public core::indexer::IMetadataExtractor {
public:
    void addExtractor(std::shared_ptr<core::indexer::IMetadataExtractor> extractor);

    bool supports(core::models::MediaType type, const std::string& extension) const override;
    core::Result<core::indexer::Metadata> extract(const std::string& canonicalPath, core::models::MediaType type) override;

private:
    std::vector<std::shared_ptr<core::indexer::IMetadataExtractor>> m_extractors;
};

} // namespace mnemis::indexer
