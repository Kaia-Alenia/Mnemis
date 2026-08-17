#include "CompositeMetadataExtractor.hpp"

namespace mnemis::indexer {

void CompositeMetadataExtractor::addExtractor(std::shared_ptr<core::indexer::IMetadataExtractor> extractor) {
    if (extractor) {
        m_extractors.push_back(std::move(extractor));
    }
}

bool CompositeMetadataExtractor::supports(core::models::MediaType type, const std::string& extension) const {
    for (const auto& extractor : m_extractors) {
        if (extractor->supports(type, extension)) {
            return true;
        }
    }
    return false;
}

core::Result<core::indexer::Metadata> CompositeMetadataExtractor::extract(const std::string& canonicalPath, core::models::MediaType type) {
    // We try to find the first extractor that supports the type.
    for (const auto& extractor : m_extractors) {
        if (extractor->supports(type, "")) {
            return extractor->extract(canonicalPath, type);
        }
    }
    
    // If no extractor supported it, return empty metadata (or we could return an error, 
    // but missing metadata isn't necessarily a fatal error for an indexer, just means no rich info).
    // The design says if it fails it returns MetadataError. Let's return error if no extractor found,
    // though the Indexer already checks supports() usually.
    return core::Error{-1, "No suitable metadata extractor found for the given media type."};
}

} // namespace mnemis::indexer
