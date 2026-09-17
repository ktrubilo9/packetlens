#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <packetlens/cli_options.hpp>
#include <packetlens/packet_source.hpp>
#include <packetlens/socket_source.hpp>

#include <memory>

namespace packetlens {
    /**
     * @brief Main application orchestrator for packet processing
     */
    class Application{
        std::unique_ptr<PacketSource> source_;
    public:
        Application() = default;

        /**
         * @param options Parsed command-line options
         * @return Exit code (0 for success, non-zero for errors)
         */
        int run(const CliOptions& options);
    };
}

#endif