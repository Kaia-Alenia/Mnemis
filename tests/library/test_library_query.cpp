#include "core/logging/Logger.hpp"

#include "database/DatabaseConnection.hpp"
#include "database/MediaRepository.hpp"
#include "database/MigrationManager.hpp"

#include "filesystem/StdFileSystem.hpp"

#include "indexer/Indexer.hpp"

#include "library/FolderRepository.hpp"
#include "library/LibraryRootRepository.hpp"
#include "library/query/MediaQuery.hpp"

#include "media/classifier/MediaClassifier.hpp"
#include "media/models/MediaType.hpp"

#include <chrono>
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
        "mnemis_phase10_" +
        std::to_string(value);
}

}

int main()
{
    bool ok = true;

    // --------------------------------------------------------
    // Temporary library
    // --------------------------------------------------------

    const fs::path root =
        fs::temp_directory_path() /
        uniqueName();

    std::error_code ec;

    fs::create_directories(
        root / "Pictures" / "Sprites",
        ec
    );

    fs::create_directories(
        root / "Music" / "Albums",
        ec
    );

    if (ec) {
        std::cerr
            << "Unable to create test root: "
            << ec.message()
            << '\n';

        return 1;
    }

    std::ofstream(
        root / "Pictures" / "hero.png"
    ) << "image";

    std::ofstream(
        root / "Pictures" / "Sprites" / "enemy.png"
    ) << "image";

    std::ofstream(
        root / "Pictures" / "wall.png"
    ) << "image";

    std::ofstream(
        root / "Music" / "Albums" / "theme.mp3"
    ) << "audio";

    std::ofstream(
        root / "movie.mp4"
    ) << "video";

    // --------------------------------------------------------
    // Database
    // --------------------------------------------------------

    mnemis::database::DatabaseConnection database;

    auto openResult =
        database.open(":memory:");

    ok &= check(
        openResult.isSuccess(),
        "Open database"
    );

    mnemis::core::Logger logger;

    mnemis::database::MigrationManager migrations(
        database,
        logger
    );

    auto migrationResult =
        migrations.migrate();

    ok &= check(
        migrationResult.isSuccess(),
        "Apply database migrations"
    );

    ok &= check(
        database.userVersion() == 3,
        "Schema version is 3"
    );

    if (!ok) {
        fs::remove_all(root, ec);
        return 1;
    }

    // --------------------------------------------------------
    // Library root
    // --------------------------------------------------------

    mnemis::library::LibraryRootRepository rootRepository(
        database
    );

    auto rootId =
        rootRepository.add(
            mnemis::filesystem::Path(
                root.string()
            ),
            "Test Library"
        );

    ok &= check(
        rootId.isSuccess(),
        "Create library root"
    );

    if (!rootId.isSuccess()) {
        fs::remove_all(root, ec);
        return 1;
    }

    // --------------------------------------------------------
    // Indexer
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

    mnemis::indexer::Indexer indexer(
        filesystem,
        classifier,
        repository,
        folders,
        logger
    );

    const mnemis::library::LibraryRootId indexRootId =
        rootId.value();

    auto rootFolder =
        folders.upsert(
            indexRootId,
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

    auto indexResult =
        indexer.indexRoot(
            indexRootId,
            mnemis::filesystem::Path(
                root.string()
            )
        );

    ok &= check(
        indexResult.isSuccess(),
        "Index test library"
    );

    // --------------------------------------------------------
    // Query layer
    // --------------------------------------------------------

    mnemis::library::MediaQuery query(
        database
    );

    // Whole root
    auto rootMedia =
        query.listInRoot(
            mnemis::filesystem::Path(
                root.string()
            ),
            true
        );

    ok &= check(
        rootMedia.isSuccess(),
        "Recursive root query"
    );

    if (rootMedia.isSuccess()) {
        ok &= check(
            rootMedia.value().size() == 5,
            "Root contains five media files"
        );
    }

    // Pictures recursive
    auto picturesRecursive =
        query.listInFolder(
            mnemis::filesystem::Path(
                (root / "Pictures").string()
            ),
            true
        );

    ok &= check(
        picturesRecursive.isSuccess(),
        "Recursive Pictures query"
    );

    if (picturesRecursive.isSuccess()) {
        ok &= check(
            picturesRecursive.value().size() == 3,
            "Pictures recursive query returns three files"
        );
    }

    // Pictures direct
    auto picturesDirect =
        query.listInFolder(
            mnemis::filesystem::Path(
                (root / "Pictures").string()
            ),
            false
        );

    ok &= check(
        picturesDirect.isSuccess(),
        "Direct Pictures query"
    );

    if (picturesDirect.isSuccess()) {
        ok &= check(
            picturesDirect.value().size() == 2,
            "Pictures direct query returns two files"
        );
    }

    // Sprites
    auto sprites =
        query.listInFolder(
            mnemis::filesystem::Path(
                (root / "Pictures" / "Sprites").string()
            ),
            true
        );

    ok &= check(
        sprites.isSuccess(),
        "Sprites query"
    );

    if (sprites.isSuccess()) {
        ok &= check(
            sprites.value().size() == 1,
            "Sprites contains one file"
        );

        if (!sprites.value().empty()) {
            ok &= check(
                sprites.value().front().fileName ==
                    "enemy.png",
                "Sprites returns enemy.png"
            );
        }
    }

    // --------------------------------------------------------
    // Search
    // --------------------------------------------------------

    mnemis::library::MediaQueryOptions searchOptions;

    searchOptions.scopePath =
        mnemis::filesystem::Path(
            (root / "Pictures").string()
        );

    searchOptions.recursive = true;
    searchOptions.searchText = "hero";

    auto searchResult =
        query.execute(
            searchOptions
        );

    ok &= check(
        searchResult.isSuccess(),
        "Search query succeeds"
    );

    if (searchResult.isSuccess()) {
        ok &= check(
            searchResult.value().size() == 1,
            "Search finds hero.png"
        );

        if (!searchResult.value().empty()) {
            ok &= check(
                searchResult.value().front().fileName ==
                    "hero.png",
                "Search result is hero.png"
            );
        }
    }

    // --------------------------------------------------------
    // Media type filter
    // --------------------------------------------------------

    mnemis::library::MediaQueryOptions videoOptions;

    videoOptions.mediaType =
        mnemis::media::MediaType::Video;

    auto videoResult =
        query.execute(
            videoOptions
        );

    ok &= check(
        videoResult.isSuccess(),
        "Video filter succeeds"
    );

    if (videoResult.isSuccess()) {
        ok &= check(
            videoResult.value().size() == 1,
            "Video filter returns one video"
        );
    }

    // --------------------------------------------------------
    // Sorting
    // --------------------------------------------------------

    mnemis::library::MediaQueryOptions sortOptions;

    sortOptions.scopePath =
        mnemis::filesystem::Path(
            (root / "Pictures").string()
        );

    sortOptions.recursive = true;
    sortOptions.sortField =
        mnemis::library::MediaSortField::Name;

    sortOptions.direction =
        mnemis::library::SortDirection::Ascending;

    auto sorted =
        query.execute(sortOptions);

    ok &= check(
        sorted.isSuccess(),
        "Ascending sort succeeds"
    );

    if (sorted.isSuccess() &&
        sorted.value().size() == 3) {

        ok &= check(
            sorted.value()[0].fileName ==
                "enemy.png",
            "Ascending sort first item"
        );

        ok &= check(
            sorted.value()[2].fileName ==
                "wall.png",
            "Ascending sort last item"
        );
    }

    sortOptions.direction =
        mnemis::library::SortDirection::Descending;

    auto reversed =
        query.execute(sortOptions);

    ok &= check(
        reversed.isSuccess(),
        "Descending sort succeeds"
    );

    if (reversed.isSuccess() &&
        reversed.value().size() == 3) {

        ok &= check(
            reversed.value()[0].fileName ==
                "wall.png",
            "Descending sort first item"
        );
    }

    // --------------------------------------------------------
    // Pagination
    // --------------------------------------------------------

    mnemis::library::MediaQueryOptions pageOptions;

    pageOptions.scopePath =
        mnemis::filesystem::Path(
            (root / "Pictures").string()
        );

    pageOptions.recursive = true;
    pageOptions.sortField =
        mnemis::library::MediaSortField::Name;

    pageOptions.limit = 1;
    pageOptions.offset = 1;

    auto page =
        query.execute(pageOptions);

    ok &= check(
        page.isSuccess(),
        "Pagination succeeds"
    );

    if (page.isSuccess()) {
        ok &= check(
            page.value().size() == 1,
            "Pagination returns one item"
        );
    }

    // --------------------------------------------------------
    // Count
    // --------------------------------------------------------

    mnemis::library::MediaQueryOptions countOptions;

    countOptions.scopePath =
        mnemis::filesystem::Path(
            (root / "Pictures").string()
        );

    countOptions.recursive = true;

    auto countResult =
        query.count(countOptions);

    ok &= check(
        countResult.isSuccess(),
        "Query count succeeds"
    );

    if (countResult.isSuccess()) {
        ok &= check(
            countResult.value() == 3,
            "Pictures count is three"
        );
    }

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------

    fs::remove_all(
        root,
        ec
    );

    ok &= check(
        !ec,
        "Cleanup test library"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Library query tests passed."
        << '\n';

    return 0;
}
