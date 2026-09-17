#ifndef RAW_FRAME_HPP
#define RAW_FRAME_HPP

#include <cstdint>
#include <vector>
#include <chrono>

namespace packetlens {
    class RawFrame {
    public:
        std::vector<std::uint8_t> data;
        std::chrono::system_clock::time_point timestamp;
    };
}

#endif