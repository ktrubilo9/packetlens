#include <filesystem>
#include <iostream>
#include <packetlens/cli/cli.hpp>
#include <packetlens/application.hpp>

using packetlens::CliParser;

int run(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    try {
        run(argc, argv);
    } catch(const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return -1;
    }
    return 0;
}

int run(int argc, char* argv[]) {
    const std::string PROGRAM_NAME = std::filesystem::path(argv[0]).filename().string();

    const auto options = CliParser::parse(argc, argv);

    using packetlens::Application;
        
    if (options.showHelp) {
        CliParser::printHelp(PROGRAM_NAME);
        return 0;
    }

    if (options.showVersion) {
        CliParser::printVersion(PROGRAM_NAME, PACKETLENS_VERSION);
        return 0;
    }

    Application app;
    return app.run(options);
}