#include <filesystem>
#include <iostream>
#include <packetlens/cli.hpp>

int main(int argc, char* argv[]) {
    const std::string PROGRAM_NAME = std::filesystem::path(argv[0]).filename().string();

    packetlens::CliParser::printVersion(PROGRAM_NAME, PACKETLENS_VERSION);
    return 0;
}