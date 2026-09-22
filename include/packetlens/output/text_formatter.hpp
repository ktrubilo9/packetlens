#ifndef TEXT_FORMATTER_HPP
#define TEXT_FORMATTER_HPP

#include <packetlens/capture/raw_frame.hpp>
#include <packetlens/decode/decoded_packet.hpp>
#include <string>

namespace packetlens {
    class TextFormatter {
    public:
        static std::string format(const RawFrame& frame, const DecodedPacket& packet);
    };
}

#endif