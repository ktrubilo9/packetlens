#ifndef DECODER_HPP
#define DECODER_HPP

#include <packetlens/capture/raw_frame.hpp>
#include <packetlens/decode/decoded_packet.hpp>

namespace packetlens {
    /// Ethernet II header consists of destination MAC, source MAC and EtherType.
    constexpr std::size_t kEthernetHeaderSize = 14;
    constexpr std::size_t kEthernetAddressSize = 6;

    /// Minimum IPv4 header size. (defined by IHL)
    constexpr std::size_t kMinimumIpv4HeaderSize = 20;

    /// Minimum TCP header size. (defined by data offset)
    constexpr std::size_t kMinimumTcpHeaderSize = 20;

    /// UDP fixed 8-byte header
    constexpr std::size_t kUdpHeaderSize = 8;

    /// Ethernet EtherType values.
    constexpr std::uint16_t kEtherTypeIpv4 = 0x0800;
    constexpr std::uint16_t kEtherTypeIpv6 = 0x86DD;
    constexpr std::uint16_t kEtherTypeArp  = 0x0806;

    /// IPv4 protocol numbers.
    constexpr std::uint8_t kIpProtocolIcmp = 1;
    constexpr std::uint8_t kIpProtocolTcp  = 6;
    constexpr std::uint8_t kIpProtocolUdp  = 17;

    class Decoder{
        /// Returns false when the frame is too short to contain complete Ethernet II header.
        bool decode_ethernet(const std::uint8_t* data, std::size_t size, DecodedPacket& result) const;
        void decode_ipv4(const std::uint8_t* data, std::size_t size, DecodedPacket& result) const;
    public:
        DecodedPacket decode(const RawFrame& frame) const;
    };
}

#endif