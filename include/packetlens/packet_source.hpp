#ifndef PACKET_SOURCE_HPP
#define PACKET_SOURCE_HPP

#include <string_view>
#include <vector>

namespace packetlens {
    /**
    * @brief Abstract class for packet input sources.
    * Defines the interface for receiving packets from
    * various sources (network interfaces, PCAP files, etc.)
    */
    class PacketSource {
    public:
        virtual ~PacketSource() = default;
        
        virtual void open() = 0;
        virtual void receive(std::vector<uint8_t>& packet) = 0;
        virtual void close() = 0;
    };
}

#endif