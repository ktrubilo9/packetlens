cmake_minimum_required(VERSION 3.19)
project(packetlens VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(packetlens 
    src/main.cpp
    src/packetlens/cli.cpp
    src/packetlens/application.cpp
    src/packetlens/socket_source.cpp
    src/packetlens/decoder.cpp
)

target_include_directories(packetlens PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(packetlens PRIVATE
    PACKETLENS_VERSION="${PROJECT_VERSION}"
)

target_compile_options(packetlens PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

include(CTest)
if(BUILD_TESTING)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/Catch2/CMakeLists.txt")
        message(FATAL_ERROR
            "Catch2 is missing. Run: git submodule update --init --recursive "
            "or configure with -DBUILD_TESTING=OFF.")
    endif()
    add_subdirectory(external/Catch2 EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
endif()
