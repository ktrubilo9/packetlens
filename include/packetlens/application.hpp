#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <packetlens/cli/cli_options.hpp>
#include <packetlens/decode/decoder.hpp>
#include <packetlens/decode/decoded_packet.hpp>
#include <packetlens/capture/packet_source.hpp>
#include <packetlens/capture/socket_source.hpp>
#include <packetlens/capture/pcap_source.hpp>

#include <memory>

namespace packetlens {
    /**
     * @brief Main application orchestrator for packet processing
     */
    class Application{
        std::unique_ptr<PacketSource> source_;
        std::unique_ptr<Decoder> decoder_;
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