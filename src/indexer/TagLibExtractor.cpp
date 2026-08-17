#include "TagLibExtractor.hpp"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>

namespace mnemis::indexer {

bool TagLibExtractor::supports(core::models::MediaType type, const std::string& extension) const {
    return type == core::models::MediaType::Audio;
}

core::Result<core::indexer::Metadata> TagLibExtractor::extract(const std::string& canonicalPath, core::models::MediaType type) {
    if (!supports(type, "")) {
        return core::Error{-1, "Unsupported media type"};
    }

#ifdef _WIN32
    TagLib::FileRef f(reinterpret_cast<const wchar_t*>(canonicalPath.c_str())); // Needs proper utf-8 to wchar_t on Windows typically, but let's assume UTF-8 is fine for standard Linux
#else
    TagLib::FileRef f(canonicalPath.c_str());
#endif

    if (f.isNull() || !f.file() || !f.file()->isValid()) {
        return core::Error{-1, "TagLib failed to parse file or file is corrupted."};
    }

    core::indexer::Metadata meta;

    if (TagLib::Tag* tag = f.tag()) {
        if (!tag->title().isEmpty()) meta.title = tag->title().to8Bit(true);
        if (!tag->artist().isEmpty()) meta.artist = tag->artist().to8Bit(true);
        if (!tag->album().isEmpty()) meta.album = tag->album().to8Bit(true);
        if (!tag->genre().isEmpty()) meta.genre = tag->genre().to8Bit(true);
        if (tag->track() > 0) meta.trackNumber = tag->track();
        if (tag->year() > 0) meta.year = tag->year();
    }

    if (TagLib::AudioProperties* properties = f.audioProperties()) {
        meta.duration = static_cast<double>(properties->lengthInMilliseconds()) / 1000.0;
        meta.audioChannels = properties->channels();
        meta.audioSampleRate = properties->sampleRate();
    }

    return meta;
}

} // namespace mnemis::indexer
