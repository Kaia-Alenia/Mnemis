#include "filesystem/StdFileSystem.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

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

std::string uniqueName()
{
    const auto now =
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count();

    return "mnemis_phase05_" +
           std::to_string(now);
}

}

int main()
{
    bool ok = true;

    const fs::path root =
        fs::temp_directory_path() /
        uniqueName();

    std::error_code cleanupError;
    fs::remove_all(root, cleanupError);

    mnemis::core::Logger logger;

    mnemis::filesystem::StdFileSystem filesystem(
        &logger
    );

    // --------------------------------------------------------
    // Path tests
    // --------------------------------------------------------

    const mnemis::filesystem::Path rawPath(
        "/tmp/mnemis/alpha/../beta/file.png"
    );

    const auto normalized =
        rawPath.normalized();

    ok &= check(
        normalized.string() ==
            "/tmp/mnemis/beta/file.png",
        "Path normalization"
    );

    ok &= check(
        normalized.fileName() ==
            "file.png",
        "Path filename extraction"
    );

    ok &= check(
        normalized.extension() ==
            ".png",
        "Path extension extraction"
    );

    ok &= check(
        normalized.parent().string() ==
            "/tmp/mnemis/beta",
        "Path parent extraction"
    );

    ok &= check(
        normalized.isAbsolute(),
        "Absolute path detection"
    );

    // --------------------------------------------------------
    // Directory creation
    // --------------------------------------------------------

    const mnemis::filesystem::Path nested(
        (root / "Pictures" / "Sprites").string()
    );

    auto createResult =
        filesystem.createDirectories(
            nested
        );

    ok &= check(
        createResult.isSuccess(),
        "Create nested directories"
    );

    auto directoryResult =
        filesystem.isDirectory(nested);

    ok &= check(
        directoryResult.isSuccess() &&
        directoryResult.value(),
        "Created path is a directory"
    );

    // --------------------------------------------------------
    // Folder model
    // --------------------------------------------------------

    auto folderResult =
        filesystem.folderInfo(nested);

    ok &= check(
        folderResult.isSuccess(),
        "Folder info succeeds"
    );

    if (folderResult.isSuccess()) {
        const auto& folder =
            folderResult.value();

        ok &= check(
            folder.isValid(),
            "Folder is valid"
        );

        ok &= check(
            folder.name == "Sprites",
            "Folder name"
        );

        ok &= check(
            !folder.isRoot,
            "Nested folder is not root"
        );
    }

    // --------------------------------------------------------
    // Files
    // --------------------------------------------------------

    const fs::path image1 =
        root / "Pictures" / "image01.png";

    const fs::path image2 =
        root / "Pictures" / "Sprites" / "hero.png";

    const fs::path textFile =
        root / "Pictures" / "Sprites" / "notes.txt";

    std::ofstream(image1).put('a');
    std::ofstream(image2).put('b');
    std::ofstream(textFile).put('c');

    // --------------------------------------------------------
    // Recursive scan
    // --------------------------------------------------------

    std::vector<mnemis::filesystem::FileInfo> files;

    const mnemis::filesystem::Path rootPath(
        root.string()
    );

    auto scanResult =
        filesystem.scanRecursive(
            rootPath,
            [&files](
                const mnemis::filesystem::FileInfo& info
            ) {
                files.push_back(info);
                return true;
            }
        );

    ok &= check(
        scanResult.isSuccess(),
        "Recursive filesystem scan"
    );

    ok &= check(
        files.size() == 3,
        "Recursive scan finds all regular files"
    );

    bool foundHero = false;
    bool foundImage = false;
    bool foundNotes = false;

    for (const auto& file : files) {
        if (file.fileName == "hero.png") {
            foundHero = true;

            ok &= check(
                file.extension == ".png",
                "hero.png extension"
            );
        }

        if (file.fileName == "image01.png") {
            foundImage = true;
        }

        if (file.fileName == "notes.txt") {
            foundNotes = true;
        }

        ok &= check(
            file.isValid(),
            "Scanned FileInfo is valid"
        );
    }

    ok &= check(
        foundHero,
        "hero.png discovered"
    );

    ok &= check(
        foundImage,
        "image01.png discovered"
    );

    ok &= check(
        foundNotes,
        "notes.txt discovered"
    );

    // --------------------------------------------------------
    // Missing path
    // --------------------------------------------------------

    const auto missingResult =
        filesystem.exists(
            mnemis::filesystem::Path(
                (root / "does-not-exist").string()
            )
        );

    ok &= check(
        missingResult.isSuccess() &&
        !missingResult.value(),
        "Missing path reports false"
    );

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------

    fs::remove_all(
        root,
        cleanupError
    );

    ok &= check(
        !cleanupError,
        "Temporary test directory cleanup"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Filesystem foundation tests passed."
        << '\n';

    return 0;
}
