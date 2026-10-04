#ifndef CLI_OPTIONS_HPP
#define CLI_OPTIONS_HPP

#include <string>

namespace packetlens{ 
    class CliOptions{
    public:
        std::string interface;
        std::string inputFile;

        std::size_t packet_count{0};

        bool showHelp{false};
        bool showVersion{false};
    };
}

#endif
