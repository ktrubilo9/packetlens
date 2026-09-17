cmake_minimum_required(VERSION 3.10)
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