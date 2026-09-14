#include <packetlens/cli_options.hpp>
#include <string_view>
#include <iostream>

namespace packetlens{ 
    
    class CliParser{
    public:
        static CliOptions parse(int argc, char* argv[]);

        static void printHelp(std::string_view programName);
        static void printVersion(std::string_view programName, std::string_view version);
    };
}