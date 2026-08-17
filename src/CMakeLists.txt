# Mnemis Plugin API
add_library(mnemis_plugin_api STATIC plugins/api/PluginInterface.cpp)
target_include_directories(mnemis_plugin_api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/plugins/api)

# Mnemis Core
add_library(mnemis_core STATIC 
    core/ApplicationContext.cpp
)
target_include_directories(mnemis_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_core PUBLIC mnemis_plugin_api)

# Mnemis Database
add_library(mnemis_database STATIC 
    database/SQLiteDatabaseService.cpp
    database/DatabaseConnection.cpp
    database/MigrationManager.cpp
    database/repositories/SQLiteMediaRepository.cpp
    ../resources/database.qrc
)
set_target_properties(mnemis_database PROPERTIES AUTORCC ON)
target_include_directories(mnemis_database PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_database PUBLIC mnemis_core Qt6::Sql SQLite::SQLite3 Qt6::Core)

# Mnemis Filesystem
add_library(mnemis_filesystem STATIC 
    filesystem/StdFileSystem.cpp
    infrastructure/watcher/QtFileWatcher.cpp
)
set_target_properties(mnemis_filesystem PROPERTIES AUTOMOC ON)
target_include_directories(mnemis_filesystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_filesystem PUBLIC mnemis_core Qt6::Core)

# Mnemis Playback
add_library(mnemis_playback STATIC 
    playback/PlaybackEngine.cpp
    playback/LibMpvBackend.cpp
)
target_include_directories(mnemis_playback PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_playback PUBLIC mnemis_core PkgConfig::MPV Qt6::Core)

# Mnemis Indexer
add_library(mnemis_indexer STATIC 
    core/indexer/Indexer.cpp
    core/indexer/MediaClassifier.cpp
    indexer/CompositeMetadataExtractor.cpp
    indexer/StbImageExtractor.cpp
    indexer/TagLibExtractor.cpp
    indexer/FFmpegExtractor.cpp
)
target_include_directories(mnemis_indexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_indexer PUBLIC mnemis_core mnemis_database PkgConfig::FFMPEG tag)

# Mnemis Thumbnails
add_library(mnemis_thumbnails STATIC 
    infrastructure/thumbnails/DiskThumbnailCache.cpp
    infrastructure/thumbnails/StbThumbnailDecoder.cpp
    infrastructure/thumbnails/FFmpegThumbnailDecoder.cpp
    infrastructure/thumbnails/QtThumbnailEngine.cpp
)
target_include_directories(mnemis_thumbnails PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_thumbnails PUBLIC mnemis_core Qt6::Core Qt6::Gui PkgConfig::FFMPEG)

# Mnemis Media
add_library(mnemis_media STATIC 
    infrastructure/media/QtAnimatedMediaDecoder.cpp
)
target_include_directories(mnemis_media PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_media PUBLIC mnemis_core Qt6::Core Qt6::Gui)

# Mnemis UI
add_library(mnemis_ui STATIC 
    ui/UIDummy.cpp
    ui/controllers/GalleryViewModel.cpp
    ui/controllers/MediaListContext.cpp
    ui/controllers/ViewerViewModel.cpp
    ui/controllers/AnimatedMediaController.cpp
    ui/controllers/PlaybackController.cpp
    ui/components/AnimatedFrameItem.cpp
    ui/providers/AsyncThumbnailProvider.cpp
)
set_target_properties(mnemis_ui PROPERTIES AUTOMOC ON)
target_include_directories(mnemis_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ui)
target_link_libraries(mnemis_ui PUBLIC mnemis_core mnemis_thumbnails Qt6::Gui Qt6::Qml Qt6::Quick)

# Mnemis App Executable
add_executable(mnemis_app 
    app/main.cpp
    ui/qml/qml.qrc
)

set_target_properties(mnemis_app PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_link_libraries(mnemis_app PRIVATE
    mnemis_plugin_api
    mnemis_core
    mnemis_database
    mnemis_filesystem
    mnemis_playback
    mnemis_indexer
    mnemis_thumbnails
    mnemis_media
    mnemis_ui
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

# Optionally output to root bin
set_target_properties(mnemis_app PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
