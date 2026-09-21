add_executable(packetlens_tests
    cli_tests.cpp
    decoder_tests.cpp
    ${PROJECT_SOURCE_DIR}/src/packetlens/cli.cpp
    ${PROJECT_SOURCE_DIR}/src/packetlens/decoder.cpp
)
target_include_directories(packetlens_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(packetlens_tests PRIVATE Catch2::Catch2WithMain)
target_compile_options(packetlens_tests PRIVATE -Wall -Wextra -Wpedantic)

include(${PROJECT_SOURCE_DIR}/external/Catch2/extras/Catch.cmake)
catch_discover_tests(packetlens_tests)
