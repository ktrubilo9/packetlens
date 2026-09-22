#ifndef PACKET_SOURCE_HPP
#define PACKET_SOURCE_HPP

#include <packetlens/capture/raw_frame.hpp>

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
        /// Returns true for a frame, false at end of input; errors throw.
        /// On EOF or an I/O error, frame is unchanged.
        virtual bool receive(RawFrame& frame) = 0;
        virtual void close() = 0;
    };
}

#endif
