cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0135 NEW)
project(Mnemis VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform independent Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Sql)
find_package(PkgConfig REQUIRED)
find_package(SQLite3 REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil)

# FetchContent for taglib
include(FetchContent)
FetchContent_Declare(
  taglib
  URL https://github.com/taglib/taglib/releases/download/v1.13.1/taglib-1.13.1.tar.gz
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ENABLE_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(taglib)
# Make tag target export its include directories to avoid hardcoded paths in consumers
target_include_directories(tag INTERFACE 
    ${taglib_SOURCE_DIR} 
    ${taglib_SOURCE_DIR}/taglib 
    ${taglib_SOURCE_DIR}/taglib/toolkit 
    ${taglib_BINARY_DIR}
)

# FetchContent for GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable CTest
enable_testing()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)
