add_executable(packetlens_tests
    cli/cli_tests.cpp
    decode/decoder_tests.cpp
    capture/pcap_source_tests.cpp
    output/text_formatter_tests.cpp
    ${PROJECT_SOURCE_DIR}/src/packetlens/cli/cli.cpp
    ${PROJECT_SOURCE_DIR}/src/packetlens/decode/decoder.cpp
    ${PROJECT_SOURCE_DIR}/src/packetlens/capture/pcap_source.cpp
    ${PROJECT_SOURCE_DIR}/src/packetlens/output/text_formatter.cpp
)
target_include_directories(packetlens_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(packetlens_tests PRIVATE Catch2::Catch2WithMain fmt::fmt)
target_compile_options(packetlens_tests PRIVATE -Wall -Wextra -Wpedantic)

include(${PROJECT_SOURCE_DIR}/external/Catch2/extras/Catch.cmake)
catch_discover_tests(packetlens_tests)

# Replace only this executable's socket calls, so lifecycle tests need no privileges.
add_executable(socket_source_tests
    capture/socket_source_tests.cpp
    ${PROJECT_SOURCE_DIR}/src/packetlens/capture/socket_source.cpp
)
target_include_directories(socket_source_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(socket_source_tests PRIVATE Catch2::Catch2WithMain)
target_compile_options(socket_source_tests PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(socket_source_tests PRIVATE
    -Wl,--wrap=socket -Wl,--wrap=if_nametoindex -Wl,--wrap=bind
    -Wl,--wrap=setsockopt -Wl,--wrap=recv -Wl,--wrap=close
)
catch_discover_tests(socket_source_tests)
