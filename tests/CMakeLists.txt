# ============================================================
# Database tests
# ============================================================

add_executable(MnemisDatabaseTests
    database/test_database.cpp

    ../src/core/errors/Error.cpp
    ../src/core/logging/Logger.cpp

    ../src/database/DatabaseConnection.cpp
    ../src/database/MigrationManager.cpp

    ../resources/database.qrc
)

target_include_directories(MnemisDatabaseTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisDatabaseTests
    PRIVATE
        Qt6::Core
        SQLite::SQLite3
)

add_test(
    NAME MnemisDatabaseTests
    COMMAND MnemisDatabaseTests
)

# ============================================================
# Persistent storage tests
# ============================================================

add_executable(MnemisPersistentStorageTests
    database/test_persistent_storage.cpp

    ../src/core/errors/Error.cpp
    ../src/core/logging/Logger.cpp

    ../src/core/storage/AppStorage.cpp

    ../src/database/DatabaseConnection.cpp
    ../src/database/DatabaseManager.cpp
    ../src/database/MigrationManager.cpp

    ../src/library/LibraryRootRepository.cpp

    ../src/filesystem/models/Path.cpp

    ../resources/database.qrc
)

target_include_directories(MnemisPersistentStorageTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisPersistentStorageTests
    PRIVATE
        Qt6::Core
        SQLite::SQLite3
)

add_test(
    NAME MnemisPersistentStorageTests
    COMMAND MnemisPersistentStorageTests
)

# ============================================================
# Filesystem tests
# ============================================================

add_executable(MnemisFilesystemTests
    filesystem/test_filesystem.cpp

    ../src/core/errors/Error.cpp
    ../src/core/logging/Logger.cpp

    ../src/filesystem/models/Path.cpp
    ../src/filesystem/StdFileSystem.cpp
)

target_include_directories(MnemisFilesystemTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisFilesystemTests
    PRIVATE
        Qt6::Core
)

add_test(
    NAME MnemisFilesystemTests
    COMMAND MnemisFilesystemTests
)

# ============================================================
# Media classifier tests
# ============================================================

add_executable(MnemisMediaClassifierTests
    media/test_media_classifier.cpp

    ../src/core/errors/Error.cpp
    ../src/media/classifier/MediaClassifier.cpp
)

target_include_directories(MnemisMediaClassifierTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisMediaClassifierTests
    PRIVATE
        Qt6::Core
)

add_test(
    NAME MnemisMediaClassifierTests
    COMMAND MnemisMediaClassifierTests
)

# ============================================================
# Indexer tests
# ============================================================

add_executable(MnemisIndexerTests
    indexer/test_indexer.cpp

    ../src/core/errors/Error.cpp
    ../src/core/logging/Logger.cpp

    ../src/database/DatabaseConnection.cpp
    ../src/database/MigrationManager.cpp
    ../src/database/MediaRepository.cpp

    ../src/filesystem/models/Path.cpp
    ../src/filesystem/StdFileSystem.cpp

    ../src/media/classifier/MediaClassifier.cpp

    ../src/library/LibraryRootRepository.cpp
    ../src/library/FolderRepository.cpp

    ../src/indexer/Indexer.cpp

    ../resources/database.qrc
)

target_include_directories(MnemisIndexerTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisIndexerTests
    PRIVATE
        Qt6::Core
        SQLite::SQLite3
)

add_test(
    NAME MnemisIndexerTests
    COMMAND MnemisIndexerTests
)

# ============================================================
# Library root tests
# ============================================================

add_executable(MnemisLibraryRootTests
    library/test_library_roots.cpp

    ../src/core/errors/Error.cpp
    ../src/core/logging/Logger.cpp

    ../src/database/DatabaseConnection.cpp
    ../src/database/MigrationManager.cpp

    ../src/filesystem/models/Path.cpp

    ../src/library/LibraryRootRepository.cpp

    ../resources/database.qrc
)

target_include_directories(MnemisLibraryRootTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisLibraryRootTests
    PRIVATE
        Qt6::Core
        SQLite::SQLite3
)

add_test(
    NAME MnemisLibraryRootTests
    COMMAND MnemisLibraryRootTests
)

# ============================================================
# Library query tests
# ============================================================

add_executable(MnemisLibraryQueryTests
    library/test_library_query.cpp

    ../src/core/errors/Error.cpp
    ../src/core/logging/Logger.cpp

    ../src/database/DatabaseConnection.cpp
    ../src/database/MigrationManager.cpp
    ../src/database/MediaRepository.cpp

    ../src/filesystem/models/Path.cpp
    ../src/filesystem/StdFileSystem.cpp

    ../src/media/classifier/MediaClassifier.cpp

    ../src/library/LibraryRootRepository.cpp
    ../src/library/FolderRepository.cpp
    ../src/library/query/MediaQuery.cpp

    ../src/indexer/Indexer.cpp

    ../resources/database.qrc
)

target_include_directories(MnemisLibraryQueryTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisLibraryQueryTests
    PRIVATE
        Qt6::Core
        SQLite::SQLite3
)

add_test(
    NAME MnemisLibraryQueryTests
    COMMAND MnemisLibraryQueryTests
)

# ============================================================
# Library Service tests
# ============================================================

add_executable(MnemisLibraryServiceTests
    library/test_library_service.cpp

    ../src/core/errors/Error.cpp
    ../src/core/logging/Logger.cpp

    ../src/database/DatabaseConnection.cpp
    ../src/database/MigrationManager.cpp
    ../src/database/MediaRepository.cpp

    ../src/filesystem/models/Path.cpp
    ../src/filesystem/StdFileSystem.cpp

    ../src/media/classifier/MediaClassifier.cpp

    ../src/library/LibraryRootRepository.cpp
    ../src/library/FolderRepository.cpp
    ../src/library/query/MediaQuery.cpp
    ../src/library/service/LibraryService.cpp

    ../src/indexer/Indexer.cpp

    ../resources/database.qrc
)

target_include_directories(MnemisLibraryServiceTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisLibraryServiceTests
    PRIVATE
        Qt6::Core
        SQLite::SQLite3
)

add_test(
    NAME MnemisLibraryServiceTests
    COMMAND MnemisLibraryServiceTests
)

# ============================================================
# Library Controller tests
# ============================================================

add_executable(MnemisLibraryControllerTests
    ui/test_library_controller.cpp

    ../src/core/errors/Error.cpp
    ../src/core/logging/Logger.cpp

    ../src/database/DatabaseConnection.cpp
    ../src/database/MigrationManager.cpp
    ../src/database/MediaRepository.cpp

    ../src/filesystem/models/Path.cpp
    ../src/filesystem/StdFileSystem.cpp

    ../src/media/classifier/MediaClassifier.cpp
    ../src/media/thumbnails/ThumbnailService.cpp

    ../src/library/LibraryRootRepository.cpp
    ../src/library/FolderRepository.cpp
    ../src/library/query/MediaQuery.cpp
    ../src/library/service/LibraryService.cpp

    ../src/indexer/Indexer.cpp

    ../src/ui/controllers/LibraryController.cpp

    ../resources/database.qrc
)

target_include_directories(MnemisLibraryControllerTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisLibraryControllerTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        SQLite::SQLite3
)

add_test(
    NAME MnemisLibraryControllerTests
    COMMAND MnemisLibraryControllerTests
)


# ============================================================
# Thumbnail tests
# ============================================================

add_executable(MnemisThumbnailTests
    media/test_thumbnail_service.cpp

    ../src/core/errors/Error.cpp

    ../src/media/thumbnails/ThumbnailService.cpp
)

target_include_directories(MnemisThumbnailTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisThumbnailTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
)

add_test(
    NAME MnemisThumbnailTests
    COMMAND MnemisThumbnailTests
)


# ============================================================
# Thumbnail controller integration tests
# ============================================================

add_executable(MnemisThumbnailControllerTests
    ui/test_thumbnail_controller.cpp

    ../src/core/errors/Error.cpp
    ../src/core/logging/Logger.cpp

    ../src/database/DatabaseConnection.cpp
    ../src/database/MigrationManager.cpp
    ../src/database/MediaRepository.cpp

    ../src/filesystem/models/Path.cpp
    ../src/filesystem/StdFileSystem.cpp

    ../src/media/classifier/MediaClassifier.cpp
    ../src/media/thumbnails/ThumbnailService.cpp

    ../src/library/LibraryRootRepository.cpp
    ../src/library/FolderRepository.cpp
    ../src/library/query/MediaQuery.cpp
    ../src/library/service/LibraryService.cpp

    ../src/indexer/Indexer.cpp

    ../src/ui/controllers/LibraryController.cpp

    ../resources/database.qrc
)

target_include_directories(MnemisThumbnailControllerTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisThumbnailControllerTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        SQLite::SQLite3
)

add_test(
    NAME MnemisThumbnailControllerTests
    COMMAND MnemisThumbnailControllerTests
)


# ============================================================
# Image / Animation viewer controller tests
# ============================================================

add_executable(MnemisImageViewerControllerTests
    ui/test_image_viewer_controller.cpp

    ../src/ui/controllers/ImageViewerController.cpp
)

target_include_directories(MnemisImageViewerControllerTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MnemisImageViewerControllerTests
    PRIVATE
        Qt6::Core
        Qt6::Gui
)

add_test(
    NAME MnemisImageViewerControllerTests
    COMMAND MnemisImageViewerControllerTests
)
