#include <string>

namespace packetlens{ 
    class CliOptions{
    public:
        std::string interface;
        std::string inputFile;
        std::string filter;
        std::string outputFile;

        std::size_t packet_count{0};

        bool json{false};
        bool quiet{false};
        bool verbose{false};

        bool showHelp{false};
        bool showVersion{false};
    };
}