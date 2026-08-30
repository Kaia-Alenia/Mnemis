add_executable(Mnemis
    app/main.cpp

    core/config/AppConfig.cpp
    core/config/AppConfig.hpp

    core/logging/Logger.cpp
    core/logging/Logger.hpp

    core/errors/Error.cpp
    core/errors/Error.hpp
    core/errors/Result.hpp

    core/storage/AppStorage.cpp
    core/storage/AppStorage.hpp

    media/models/MediaId.hpp
    media/models/MediaType.hpp
    media/models/Media.hpp
    media/classifier/MediaClassifier.cpp
    media/classifier/MediaClassifier.hpp

    media/thumbnails/ThumbnailService.cpp
    media/thumbnails/ThumbnailService.hpp

    media/viewer/ViewerTypes.hpp

    ui/controllers/ImageViewerController.cpp
    ui/controllers/ImageViewerController.hpp

    database/DatabaseConnection.cpp
    database/DatabaseConnection.hpp
    database/DatabaseManager.cpp
    database/DatabaseManager.hpp
    database/MigrationManager.cpp
    database/MigrationManager.hpp
    database/MediaRepository.cpp
    database/MediaRepository.hpp

    filesystem/models/Path.cpp
    filesystem/models/Path.hpp
    filesystem/models/Folder.hpp
    filesystem/models/FileInfo.hpp
    filesystem/IFileSystem.hpp
    filesystem/StdFileSystem.cpp
    filesystem/StdFileSystem.hpp

    library/models/LibraryRoot.hpp
    library/LibraryRootRepository.cpp
    library/LibraryRootRepository.hpp
    library/models/Folder.hpp
    library/FolderRepository.cpp
    library/FolderRepository.hpp

    library/query/MediaSort.hpp
    library/query/MediaQuery.hpp
    library/query/MediaQuery.cpp

    library/service/LibraryService.hpp
    library/service/LibraryService.cpp

    ui/controllers/LibraryController.hpp
    ui/controllers/LibraryController.cpp

    indexer/Indexer.cpp
    indexer/Indexer.hpp

    ui/qml/qml.qrc
    ../resources/database.qrc
)

set_target_properties(Mnemis PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(Mnemis
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Mnemis
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Qml
        Qt6::Quick
        SQLite::SQLite3
)
