find_package(Qt6 COMPONENTS Test REQUIRED)

add_executable(mnemis_tests 
    main.cpp
    test_debug.cpp
    test_animated_frame_item.cpp
    test_result.cpp
    test_di_lifecycle.cpp
    test_database.cpp
    test_migration_manager.cpp
    test_media_repository.cpp
    test_database_transactions.cpp
    test_filesystem.cpp
    test_extractors.cpp
    test_indexer.cpp
    test_media_classifier.cpp
    test_qt_watcher.cpp
    test_thumbnail_cache.cpp
    test_thumbnails.cpp
    test_unicode_paths.cpp
    test_gallery_viewmodel.cpp
    test_viewerviewmodel.cpp
    test_medialistcontext.cpp
    test_animated_media_decoder.cpp
    test_animated_media_controller.cpp
    test_playback_engine.cpp
    test_playback_controller.cpp
    test_libmpv_backend.cpp
    test_app_bootstrap.cpp
    test_settings_viewmodel.cpp
)

target_link_libraries(mnemis_tests PRIVATE
    gtest
    gmock
    Qt6::Test
    mnemis_core
    mnemis_database
    mnemis_filesystem
    mnemis_playback
    mnemis_indexer
    mnemis_thumbnails
    mnemis_media
    mnemis_ui
)

target_compile_definitions(mnemis_tests PRIVATE TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

include(GoogleTest)
gtest_discover_tests(mnemis_tests)
