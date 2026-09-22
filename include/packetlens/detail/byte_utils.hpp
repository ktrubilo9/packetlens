#ifndef BYTE_UTILS_HPP
#define BYTE_UTILS_HPP

#include <cstdint>
#include <cstddef>

namespace packetlens {
    /**
     * Reads a 16-bit uint in big-endian byte order.
     * 
     * @param data Pointer to the byte buffer.
     * @param offset Offset of the first byte to read.
     */
    inline std::uint16_t read_u16_be(
        const std::uint8_t* data,
        std::size_t offset
    ) {
        return (static_cast<std::uint16_t>(data[offset]) << 8)
            | static_cast<std::uint16_t>(data[offset + 1]);
    }

    /**
     * Reads a 32-bit uint from a buffer in big-endian byte order.
     * 
     * @param data Pointer to the byte buffer.
     * @param offset Offset of the first byte to read.
     */
    inline std::uint32_t read_u32_be(
        const std::uint8_t* data,
        std::size_t offset
    ) {
        return (static_cast<std::uint32_t>(data[offset]) << 24)
            | (static_cast<std::uint32_t>(data[offset + 1]) << 16)
            | (static_cast<std::uint32_t>(data[offset + 2]) << 8)
            | static_cast<std::uint32_t>(data[offset+3]);
    }

    inline std::uint16_t read_u16_le(
        const std::uint8_t* data,
        std::size_t offset
    ) {
        return static_cast<std::uint16_t>(data[offset])
            | (static_cast<std::uint16_t>(data[offset + 1]) << 8);
    }

    inline std::uint32_t read_u32_le(
        const std::uint8_t* data,
        std::size_t offset
    ) {
        return static_cast<std::uint32_t>(data[offset])
            | (static_cast<std::uint32_t>(data[offset + 1]) << 8)
            | (static_cast<std::uint32_t>(data[offset + 2]) << 16)
            | (static_cast<std::uint32_t>(data[offset + 3]) << 24);
    }
}

#endif