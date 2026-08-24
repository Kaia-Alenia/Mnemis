#include "media/classifier/MediaClassifier.hpp"

#include <iostream>
#include <string>

namespace {

bool check(
    bool condition,
    const char* message
)
{
    if (!condition) {
        std::cerr
            << "FAIL: "
            << message
            << '\n';

        return false;
    }

    return true;
}

}

int main()
{
    bool ok = true;

    const mnemis::media::MediaClassifier classifier;

    using mnemis::media::MediaType;

    // --------------------------------------------------------
    // Images
    // --------------------------------------------------------

    ok &= check(
        classifier.classifyExtension(".png") ==
            MediaType::Image,
        "PNG classified as image"
    );

    ok &= check(
        classifier.classifyExtension(".JPG") ==
            MediaType::Image,
        "JPG classified case-insensitively"
    );

    ok &= check(
        classifier.classifyExtension("jpeg") ==
            MediaType::Image,
        "JPEG without leading dot classified"
    );

    ok &= check(
        classifier.classifyExtension(".GIF") ==
            MediaType::Image,
        "GIF classified as image"
    );

    ok &= check(
        classifier.classifyExtension(".webp") ==
            MediaType::Image,
        "WEBP classified as image"
    );

    // --------------------------------------------------------
    // Video
    // --------------------------------------------------------

    ok &= check(
        classifier.classifyExtension(".mp4") ==
            MediaType::Video,
        "MP4 classified as video"
    );

    ok &= check(
        classifier.classifyExtension(".MKV") ==
            MediaType::Video,
        "MKV classified case-insensitively"
    );

    ok &= check(
        classifier.classifyExtension("webm") ==
            MediaType::Video,
        "WEBM without leading dot classified"
    );

    ok &= check(
        classifier.classifyExtension(".mov") ==
            MediaType::Video,
        "MOV classified as video"
    );

    // --------------------------------------------------------
    // Audio
    // --------------------------------------------------------

    ok &= check(
        classifier.classifyExtension(".mp3") ==
            MediaType::Audio,
        "MP3 classified as audio"
    );

    ok &= check(
        classifier.classifyExtension(".FLAC") ==
            MediaType::Audio,
        "FLAC classified case-insensitively"
    );

    ok &= check(
        classifier.classifyExtension("wav") ==
            MediaType::Audio,
        "WAV without leading dot classified"
    );

    ok &= check(
        classifier.classifyExtension(".opus") ==
            MediaType::Audio,
        "OPUS classified as audio"
    );

    // --------------------------------------------------------
    // Unknown
    // --------------------------------------------------------

    ok &= check(
        classifier.classifyExtension(".pdf") ==
            MediaType::Unknown,
        "PDF classified as unknown"
    );

    ok &= check(
        classifier.classifyExtension(".zip") ==
            MediaType::Unknown,
        "ZIP classified as unknown"
    );

    ok &= check(
        classifier.classifyExtension("") ==
            MediaType::Unknown,
        "Empty extension classified as unknown"
    );

    // --------------------------------------------------------
    // Supported extension
    // --------------------------------------------------------

    ok &= check(
        classifier.isSupportedExtension(".png"),
        "PNG reported as supported"
    );

    ok &= check(
        classifier.isSupportedExtension(".mp4"),
        "MP4 reported as supported"
    );

    ok &= check(
        classifier.isSupportedExtension(".mp3"),
        "MP3 reported as supported"
    );

    ok &= check(
        !classifier.isSupportedExtension(".pdf"),
        "PDF reported as unsupported"
    );

    // --------------------------------------------------------
    // Path classification
    // --------------------------------------------------------

    auto imageResult =
        classifier.classifyPath(
            "/tmp/assets/hero.PNG"
        );

    ok &= check(
        imageResult.isSuccess(),
        "Path image classification succeeds"
    );

    if (imageResult.isSuccess()) {
        ok &= check(
            imageResult.value() == MediaType::Image,
            "Path image classified correctly"
        );
    }

    auto videoResult =
        classifier.classifyPath(
            "/tmp/videos/demo.MKV"
        );

    ok &= check(
        videoResult.isSuccess(),
        "Path video classification succeeds"
    );

    if (videoResult.isSuccess()) {
        ok &= check(
            videoResult.value() == MediaType::Video,
            "Path video classified correctly"
        );
    }

    auto audioResult =
        classifier.classifyPath(
            "/tmp/music/theme.FLAC"
        );

    ok &= check(
        audioResult.isSuccess(),
        "Path audio classification succeeds"
    );

    if (audioResult.isSuccess()) {
        ok &= check(
            audioResult.value() == MediaType::Audio,
            "Path audio classified correctly"
        );
    }

    auto unknownResult =
        classifier.classifyPath(
            "/tmp/documents/readme.pdf"
        );

    ok &= check(
        unknownResult.isError(),
        "Unsupported path returns an error"
    );

    auto missingExtensionResult =
        classifier.classifyPath(
            "/tmp/media/no_extension"
        );

    ok &= check(
        missingExtensionResult.isError(),
        "Path without extension returns an error"
    );

    auto emptyPathResult =
        classifier.classifyPath("");

    ok &= check(
        emptyPathResult.isError(),
        "Empty path returns an error"
    );

    // --------------------------------------------------------
    // Extension lists
    // --------------------------------------------------------

    const auto images =
        classifier.imageExtensions();

    const auto videos =
        classifier.videoExtensions();

    const auto audio =
        classifier.audioExtensions();

    ok &= check(
        !images.empty(),
        "Image extension list is not empty"
    );

    ok &= check(
        !videos.empty(),
        "Video extension list is not empty"
    );

    ok &= check(
        !audio.empty(),
        "Audio extension list is not empty"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Media classifier tests passed."
        << '\n';

    return 0;
}
