#ifndef DECODER_HPP
#define DECODER_HPP

#include <packetlens/raw_frame.hpp>
#include <packetlens/decoded_packet.hpp>

namespace packetlens {
    class Decoder{
    public:
        DecodedPacket decode(const RawFrame& frame) const;
    };
}

#endif