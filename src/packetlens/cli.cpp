#include <packetlens/cli.hpp>

using packetlens::CliOptions;
using packetlens::CliParser;

CliOptions CliParser::parse(int argc, char* argv[]) {

    return {};
}

void CliParser::printHelp(std::string_view programName)
{
    std::cout
        << "Usage: " << programName << " [OPTIONS]\n"
        << "\n"
        << "Options:\n"
        << "  -i, --interface <name>   Network interface to capture from\n"
        << "  -f, --filter <expr>      Capture filter\n"
        << "  -c, --count <number>     Stop after N packets\n"
        << "  -h, --help               Show this help\n"
        << "  -V, --version            Show version\n";
}

void CliParser::printVersion(std::string_view programName, std::string_view version)
{
    std::cout << programName << " " << version << "\n";
}