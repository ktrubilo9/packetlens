#ifndef CLI_HPP
#define CLI_HPP

#include <packetlens/cli_options.hpp>
#include <string_view>

namespace packetlens{ 
    
    class CliParser{
        static void validate(const CliOptions& options);
    public:
        static CliOptions parse(int argc, char* argv[]);

        static void printHelp(std::string_view programName);
        static void printVersion(std::string_view programName, std::string_view version);
    };
}

#endif