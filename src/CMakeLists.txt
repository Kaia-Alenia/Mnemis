# Mnemis Plugin API
add_library(mnemis_plugin_api STATIC plugins/api/PluginInterface.cpp)
target_include_directories(mnemis_plugin_api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/plugins/api)

# Mnemis Core
add_library(mnemis_core STATIC 
    core/ApplicationContext.cpp
    plugins/core/PluginManager.cpp
    plugins/manifest/PluginManifest.cpp
    plugins/loader/PluginDiscovery.cpp
)
set_target_properties(mnemis_core PROPERTIES AUTOMOC ON)
target_include_directories(mnemis_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_core PUBLIC mnemis_plugin_api Qt6::Core)

# Mnemis Database
add_library(mnemis_database STATIC 
    database/SQLiteDatabaseService.cpp
    database/DatabaseConnection.cpp
    database/MigrationManager.cpp
    database/repositories/SQLiteMediaRepository.cpp
    database/repositories/SQLitePlaylistRepository.cpp
    ../resources/database.qrc
)
set_target_properties(mnemis_database PROPERTIES AUTORCC ON)
target_include_directories(mnemis_database PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_database PUBLIC mnemis_core Qt6::Sql SQLite::SQLite3 Qt6::Core)

# Mnemis Filesystem
add_library(mnemis_filesystem STATIC 
    filesystem/StdFileSystem.cpp
    infrastructure/watcher/QtFileWatcher.cpp
    infrastructure/config/QtConfig.cpp
    infrastructure/bootstrap/DefaultLibraryRoots.cpp
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
    ui/controllers/PlaylistController.cpp
    ui/controllers/PlaybackController.cpp
    ui/controllers/SettingsViewModel.cpp
    ui/components/AnimatedFrameItem.cpp
    ui/components/MpvVideoItem.cpp
    ui/providers/AsyncThumbnailProvider.cpp
)
set_target_properties(mnemis_ui PROPERTIES AUTOMOC ON)
target_include_directories(mnemis_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ui)
target_link_libraries(mnemis_ui PUBLIC mnemis_core mnemis_thumbnails mnemis_media Qt6::Gui Qt6::Qml Qt6::Quick)

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
    Qt6::Widgets
)

# Optionally output to root bin
set_target_properties(mnemis_app PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Mnemis Test Plugin
add_library(mnemis_test_plugin MODULE 
    plugins/test_plugin/TestPlugin.cpp
)
set_target_properties(mnemis_test_plugin PROPERTIES AUTOMOC ON)
target_include_directories(mnemis_test_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_test_plugin PRIVATE mnemis_plugin_api Qt6::Core)
set_target_properties(mnemis_test_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/test_plugin")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/plugins/test_plugin/manifest.json DESTINATION "${CMAKE_BINARY_DIR}/bin/plugins/test_plugin")

# Mnemis KoalaSync Plugin
add_library(mnemis_koalasync MODULE 
    plugins/koalasync_plugin/KoalaSyncPlugin.cpp
    plugins/koalasync_plugin/KoalaSyncSession.cpp
    plugins/koalasync_plugin/transport/WebSocketTransport.cpp
    plugins/koalasync_plugin/protocol/ProtocolAdapter.cpp
)
set_target_properties(mnemis_koalasync PROPERTIES AUTOMOC ON)
target_include_directories(mnemis_koalasync PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mnemis_koalasync PRIVATE mnemis_plugin_api Qt6::Core Qt6::WebSockets Qt6::Network)
set_target_properties(mnemis_koalasync PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/com.mnemis.koalasync")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/plugins/koalasync_plugin/manifest.json DESTINATION "${CMAKE_BINARY_DIR}/bin/plugins/com.mnemis.koalasync")

# Packaging with CPack
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "Mnemis")
set(CPACK_PACKAGE_VENDOR "Alenia Studios")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Mnemis - A robust media management application")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_CONTACT "contact.aleniastudios@gmail.com")
set(CPACK_GENERATOR "DEB;TGZ")
include(CPack)
