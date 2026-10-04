#include <packetlens/cli/cli.hpp>

#include <catch2/catch_test_macros.hpp>

#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
packetlens::CliOptions parse(std::initializer_list<std::string> arguments) {
    // getopt_long may reorder argv, and requires writable, null-terminated strings.
    std::vector<std::string> storage{"packetlens"};
    storage.insert(storage.end(), arguments.begin(), arguments.end());
    std::vector<char*> argv;
    for (auto& argument : storage) {
        argv.push_back(argument.data());
    }
    argv.push_back(nullptr);
    return packetlens::CliParser::parse(static_cast<int>(storage.size()), argv.data());
}

void check_invalid_argument(
    std::initializer_list<std::string> arguments,
    const std::string& expected_message
) {
    try {
        parse(arguments);
        FAIL("expected std::invalid_argument");
    } catch (const std::invalid_argument& error) {
        CHECK(error.what() == expected_message);
    }
}
}

TEST_CASE("CLI accepts an interface with default options", "[cli]") {
    const auto options = parse({"--interface", "eth0"});
    CHECK(options.interface == "eth0");
    CHECK(options.inputFile.empty());
    CHECK(options.packet_count == 0);
    CHECK_FALSE(options.showHelp);
    CHECK_FALSE(options.showVersion);
}

TEST_CASE("CLI parses short and long option values", "[cli]") {
    SECTION("short options") {
        const auto options = parse({"-i", "lo", "-c", "42"});
        CHECK(options.interface == "lo");
        CHECK(options.packet_count == 42);
    }
    SECTION("long options with equals syntax") {
        const auto options = parse({"--interface=eth0", "--count=42"});
        CHECK(options.interface == "eth0");
        CHECK(options.packet_count == 42);
    }
    SECTION("file input") {
        const auto options = parse({"-r", "capture.pcap"});
        CHECK(options.inputFile == "capture.pcap");
        CHECK(options.interface.empty());
    }
}

TEST_CASE("CLI explicitly rejects options planned for a later release", "[cli]") {
    check_invalid_argument(
        {"-i", "lo", "-f", "tcp"},
        "option --filter is not implemented"
    );
    check_invalid_argument(
        {"--interface=lo", "--filter=tcp"},
        "option --filter is not implemented"
    );
    check_invalid_argument(
        {"-i", "lo", "-o", "out.txt"},
        "option --output is not implemented"
    );
    check_invalid_argument(
        {"--interface=lo", "--output=out.txt"},
        "option --output is not implemented"
    );

    check_invalid_argument(
        {"-i", "lo", "-j"},
        "option --json is not implemented"
    );
    check_invalid_argument(
        {"-i", "lo", "--json"},
        "option --json is not implemented"
    );
    check_invalid_argument(
        {"-i", "lo", "-q"},
        "option --quiet is not implemented"
    );
    check_invalid_argument(
        {"-i", "lo", "--quiet"},
        "option --quiet is not implemented"
    );
    check_invalid_argument(
        {"-i", "lo", "-v"},
        "option --verbose is not implemented"
    );
    check_invalid_argument(
        {"-i", "lo", "--verbose"},
        "option --verbose is not implemented"
    );
}

TEST_CASE("CLI help and version do not require a source", "[cli]") {
    CHECK(parse({"-h"}).showHelp);
    CHECK(parse({"--help"}).showHelp);
    CHECK(parse({"-V"}).showVersion);
    CHECK(parse({"--version"}).showVersion);
}

TEST_CASE("CLI rejects invalid option combinations", "[cli]") {
    CHECK_THROWS_AS(parse({}), std::invalid_argument);
    CHECK_THROWS_AS(parse({"-i", ""}), std::invalid_argument);
    CHECK_THROWS_AS(parse({"-r", ""}), std::invalid_argument);
    CHECK_THROWS_AS(parse({"-i", "lo", "-r", "capture.pcap"}), std::invalid_argument);
}

TEST_CASE("CLI rejects unknown options and missing option values", "[cli]") {
    CHECK_THROWS_AS(parse({"--unknown"}), std::invalid_argument);
    CHECK_THROWS_AS(parse({"-z"}), std::invalid_argument);
    for (const auto* option : {"-i", "--interface", "-r", "--read", "-f", "--filter",
                               "-o", "--output", "-c", "--count"}) {
        CAPTURE(option);
        CHECK_THROWS_AS(parse({"-i", "lo", option}), std::invalid_argument);
    }
}

TEST_CASE("CLI accepts packet count boundaries", "[cli]") {
    CHECK(parse({"-i", "lo", "-c", "0"}).packet_count == 0);
    CHECK(parse({"-i", "lo", "-c", "1"}).packet_count == 1);
    const auto maximum = std::numeric_limits<std::size_t>::max();
    CHECK(parse({"-i", "lo", "-c", std::to_string(maximum)}).packet_count == maximum);
}

TEST_CASE("CLI rejects malformed packet counts", "[cli]") {
    for (const auto* value : {"", "abc", "-1", "+1", "12abc", "1.5", " 1", "1 ",
                              "18446744073709551616"}) {
        CAPTURE(value);
        CHECK_THROWS_AS(parse({"-i", "lo", "-c", value}), std::invalid_argument);
    }
}

TEST_CASE("CLI rejects positional arguments", "[cli]") {
    CHECK_THROWS_AS(parse({"-i", "lo", "unexpected"}), std::invalid_argument);
    CHECK_THROWS_AS(parse({"-i", "lo", "--", "unexpected"}), std::invalid_argument);
}

TEST_CASE("CLI can be reused after successful and failed parsing", "[cli]") {
    CHECK(parse({"-i", "first", "-c", "7"}).packet_count == 7);
    CHECK_THROWS_AS(parse({"-i", "lo", "-cz"}), std::invalid_argument);
    const auto options = parse({"-i", "second"});
    CHECK(options.interface == "second");
    CHECK(options.packet_count == 0);
    // An error in a grouped option can leave getopt's internal cursor in argv.
    CHECK_THROWS_AS(parse({"-zi", "lo"}), std::invalid_argument);
    CHECK(parse({"-i", "third"}).interface == "third");
}
