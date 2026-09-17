#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <packetlens/cli_options.hpp>

namespace packetlens {
    class Application{
    public:
        Application() = default;

        int run(const CliOptions& options);
    };
}

#endif