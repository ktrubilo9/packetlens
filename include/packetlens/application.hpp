#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <packetlens/cli_options.hpp>
#include <packetlens/packet_source.hpp>
#include <packetlens/socket_source.hpp>

#include <memory>

namespace packetlens {
    class Application{
        std::unique_ptr<PacketSource> source_;
    public:
        Application() = default;

        int run(const CliOptions& options);
    };
}

#endif