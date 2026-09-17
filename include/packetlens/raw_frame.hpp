#ifndef RAW_FRAME_HPP
#define RAW_FRAME_HPP

#include <cstdint>
#include <vector>
#include <chrono>

namespace packetlens {
    /**
     * @brief Represents a raw network frame with metadata
     * 
     * RawFrame contains the raw byte data of a captured network packet
     * along with timing information.
     */
    class RawFrame {
    public:
        std::vector<std::uint8_t> data;                       ///< Raw packet bytes
        std::chrono::system_clock::time_point timestamp;      ///< Capture timestamp
    };
}

#endif