#ifndef TEXT_FORMATTER_HPP
#define TEXT_FORMATTER_HPP

#include <packetlens/raw_frame.hpp>
#include <packetlens/decoded_packet.hpp>
#include <string>

namespace packetlens {
    class TextFormatter {
    public:
        static std::string format(const RawFrame& frame, const DecodedPacket& packet);
    };
}

#endif