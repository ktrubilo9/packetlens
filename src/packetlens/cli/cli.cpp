#include <packetlens/cli/cli.hpp>

using packetlens::CliOptions;
using packetlens::CliParser;

#include <getopt.h>
#include <charconv>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
    [[noreturn]] void throw_unsupported_option(std::string_view option) {
        throw std::invalid_argument(
            "option " + std::string(option) + " is not implemented"
        );
    }
}

CliOptions CliParser::parse(int argc, char* argv[]) {
    CliOptions options;

    // GNU getopt needs zero to reset its internal cursor as well as argv indexing.
    optind = 0;

    const option longOptions[] = {
        {"interface", required_argument, nullptr, 'i'},
        {"read",      required_argument, nullptr, 'r'},
        {"filter",    required_argument, nullptr, 'f'},
        {"output",    required_argument, nullptr, 'o'},
        {"count",     required_argument, nullptr, 'c'},
        {"json",      no_argument,       nullptr, 'j'},
        {"quiet",     no_argument,       nullptr, 'q'},
        {"verbose",   no_argument,       nullptr, 'v'},
        {"help",      no_argument,       nullptr, 'h'},
        {"version",   no_argument,       nullptr, 'V'},
        {nullptr,     0,                 nullptr,  0}
    };

    constexpr const char* shortOptions = "i:r:f:o:c:jqvhV";

    while (true) {
        const int option = getopt_long(
            argc,
            argv,
            shortOptions,
            longOptions,
            nullptr
        );

        if (option == -1) {
            break;
        }

        switch (option) {
        case 'i':
            options.interface = optarg;
            break;
        case 'r':
            options.inputFile = optarg;
            break;
        case 'f':
            throw_unsupported_option("--filter");
        case 'o':
            throw_unsupported_option("--output");
        case 'c': {
            const std::string_view value(optarg);
            std::size_t count = 0;
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), count);
            if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) {
                throw std::invalid_argument(
                    "invalid packet count: " + std::string(optarg)
                );
            }
            options.packet_count = count;
            break;
        }
        case 'j':
            throw_unsupported_option("--json");
        case 'q':
            throw_unsupported_option("--quiet");
        case 'v':
            throw_unsupported_option("--verbose");
        case 'h':
            options.showHelp = true;
            break;
        case 'V':
            options.showVersion = true;
            break;
        case '?':
            if (optopt != 0) {
                throw std::invalid_argument(
                    "invalid option: -" + std::string(1, static_cast<char>(optopt))
                );
            }

            throw std::invalid_argument(
                "invalid command-line argument"
            );
        }
    }

    if (optind < argc) {
        throw std::invalid_argument("unexpected argument: " + std::string(argv[optind]));
    }

    validate_(options);

    return options;
}

void CliParser::validate_(const CliOptions& options) {
    if (options.showHelp || options.showVersion) {
        return;
    }

    const bool hasInterface = !options.interface.empty();
    const bool hasInputFile = !options.inputFile.empty();

    if (!hasInterface && !hasInputFile) {
        throw std::invalid_argument(
            "no input source specified; use --interface or --read"
        );
    }

    if (hasInterface && hasInputFile) {
        throw std::invalid_argument(
            "--interface and --read cannot be used together"
        );
    }
}

void CliParser::printHelp(std::string_view programName)
{
    std::cout
        << "Usage: " << programName << " [OPTIONS]\n"
        << "\n"
        << "Input:\n"
        << "  -i, --interface <name>   Capture packets from network interface\n"
        << "  -r, --read <file>        Read packets from PCAP file\n"
        << "\n"
        << "Other:\n"
        << "  -c, --count <number>     Stop after N packets (0 = unlimited)\n"
        << "  -h, --help               Show this help message\n"
        << "  -V, --version            Show version information\n";
}

void CliParser::printVersion(std::string_view programName, std::string_view version)
{
    std::cout << programName << " " << version << "\n";
}
