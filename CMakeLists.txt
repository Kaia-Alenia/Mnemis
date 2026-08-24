cmake_minimum_required(VERSION 3.24)

project(Mnemis
    VERSION 0.18.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(CTest)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
)

find_package(SQLite3 REQUIRED)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
