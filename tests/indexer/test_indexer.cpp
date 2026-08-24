#include "core/logging/Logger.hpp"

#include "database/DatabaseConnection.hpp"
#include "database/MediaRepository.hpp"
#include "library/FolderRepository.hpp"
#include "library/LibraryRootRepository.hpp"
#include "database/MigrationManager.hpp"

#include "filesystem/StdFileSystem.hpp"

#include "indexer/Indexer.hpp"

#include "media/classifier/MediaClassifier.hpp"
#include "media/models/MediaType.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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
    const auto value =
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count();

    return
        "mnemis_phase07_" +
        std::to_string(value);
}

}

int main()
{
    bool ok = true;

    // --------------------------------------------------------
    // Temporary filesystem
    // --------------------------------------------------------

    const fs::path root =
        fs::temp_directory_path() /
        uniqueName();

    std::error_code ec;

    fs::remove_all(
        root,
        ec
    );

    fs::create_directories(
        root / "Pictures",
        ec
    );

    if (ec) {
        std::cerr
            << "Unable to create test root: "
            << ec.message()
            << '\n';

        return 1;
    }

    // The indexer currently classifies by extension.
    // File contents are intentionally irrelevant at this stage.
    std::ofstream(
        root / "Pictures" / "hero.png"
    ) << "fake png";

    std::ofstream(
        root / "Pictures" / "movie.mp4"
    ) << "fake video";

    std::ofstream(
        root / "Pictures" / "song.mp3"
    ) << "fake audio";

    std::ofstream(
        root / "Pictures" / "notes.pdf"
    ) << "not supported";

    // --------------------------------------------------------
    // Database
    // --------------------------------------------------------

    mnemis::database::DatabaseConnection database;

    auto openResult =
        database.open(":memory:");

    ok &= check(
        openResult.isSuccess(),
        "Open test database"
    );

    if (!ok) {
        fs::remove_all(root, ec);
        return 1;
    }

    mnemis::core::Logger logger;

    mnemis::database::MigrationManager migrations(
        database,
        logger
    );

    auto migrationResult =
        migrations.migrate();

    ok &= check(
        migrationResult.isSuccess(),
        "Run database migrations"
    );

    if (!ok) {
        fs::remove_all(root, ec);
        return 1;
    }

    // --------------------------------------------------------
    // Indexing stack
    // --------------------------------------------------------

    mnemis::filesystem::StdFileSystem filesystem(
        &logger
    );

    mnemis::media::MediaClassifier classifier;

    mnemis::database::MediaRepository repository(
        database
    );

    mnemis::library::FolderRepository folders(
        database
    );

    mnemis::library::LibraryRootRepository roots(
        database
    );

    auto rootIdResult =
        roots.add(
            mnemis::filesystem::Path(
                root.string()
            ),
            "Test Library"
        );

    ok &= check(
        rootIdResult.isSuccess(),
        "Create test library root"
    );

    if (!rootIdResult.isSuccess()) {
        fs::remove_all(root, ec);
        return 1;
    }

    const mnemis::library::LibraryRootId rootId =
        rootIdResult.value();

    auto rootFolder =
        folders.upsert(
            rootId,
            mnemis::library::InvalidFolderId,
            mnemis::filesystem::Path(
                root.string()
            ),
            "Test Library",
            true
        );

    ok &= check(
        rootFolder.isSuccess(),
        "Create test root folder"
    );

    mnemis::indexer::Indexer indexer(
        filesystem,
        classifier,
        repository,
        folders,
        logger
    );

    auto indexResult =
        indexer.indexRoot(
            rootId,
            mnemis::filesystem::Path(
                root.string()
            )
        );

    ok &= check(
        indexResult.isSuccess(),
        "Indexer completed successfully"
    );

    if (indexResult.isSuccess()) {
        const auto& stats =
            indexResult.value();

        ok &= check(
            stats.scannedFiles == 4,
            "Indexer scanned four files"
        );

        ok &= check(
            stats.indexedFiles == 3,
            "Indexer indexed three supported media files"
        );

        ok &= check(
            stats.skippedFiles == 1,
            "Indexer skipped one unsupported file"
        );
    }

    // --------------------------------------------------------
    // Repository counts
    // --------------------------------------------------------

    auto totalResult =
        repository.count();

    ok &= check(
        totalResult.isSuccess(),
        "Repository total count succeeds"
    );

    if (totalResult.isSuccess()) {
        ok &= check(
            totalResult.value() == 3,
            "Repository contains three media items"
        );
    }

    auto imageCount =
        repository.countByType(
            mnemis::media::MediaType::Image
        );

    auto videoCount =
        repository.countByType(
            mnemis::media::MediaType::Video
        );

    auto audioCount =
        repository.countByType(
            mnemis::media::MediaType::Audio
        );

    ok &= check(
        imageCount.isSuccess() &&
        imageCount.value() == 1,
        "One image indexed"
    );

    ok &= check(
        videoCount.isSuccess() &&
        videoCount.value() == 1,
        "One video indexed"
    );

    ok &= check(
        audioCount.isSuccess() &&
        audioCount.value() == 1,
        "One audio item indexed"
    );

    // --------------------------------------------------------
    // Path lookup
    // --------------------------------------------------------

    const fs::path heroPath =
        root / "Pictures" / "hero.png";

    auto heroResult =
        repository.findByPath(
            heroPath.lexically_normal().string()
        );

    ok &= check(
        heroResult.isSuccess(),
        "Find indexed image by path"
    );

    if (heroResult.isSuccess()) {
        const auto& hero =
            heroResult.value();

        ok &= check(
            hero.type ==
                mnemis::media::MediaType::Image,
            "Indexed image has correct type"
        );

        ok &= check(
            hero.fileName == "hero.png",
            "Indexed image has correct filename"
        );

        ok &= check(
            hero.isValid(),
            "Indexed media is valid"
        );
    }

    // --------------------------------------------------------
    // Prefix lookup
    // --------------------------------------------------------

    auto prefixResult =
        repository.countByPrefix(
            (root / "Pictures").string()
        );

    ok &= check(
        prefixResult.isSuccess(),
        "Count by folder prefix succeeds"
    );

    if (prefixResult.isSuccess()) {
        ok &= check(
            prefixResult.value() == 3,
            "Folder prefix contains three indexed media files"
        );
    }

    // --------------------------------------------------------
    // Re-indexing must not duplicate entries
    // --------------------------------------------------------

    auto secondIndex =
        indexer.indexRoot(
            rootId,
            mnemis::filesystem::Path(
                root.string()
            )
        );

    ok &= check(
        secondIndex.isSuccess(),
        "Second indexing pass succeeds"
    );

    auto finalCount =
        repository.count();

    ok &= check(
        finalCount.isSuccess() &&
        finalCount.value() == 3,
        "Re-indexing does not create duplicates"
    );

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------

    fs::remove_all(
        root,
        ec
    );

    ok &= check(
        !ec,
        "Cleanup test directory"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Indexer tests passed."
        << '\n';

    return 0;
}
