#ifndef DECODED_PACKET_HPP
#define DECODED_PACKET_HPP

#include <array>
#include <cstdint>
#include <optional>

namespace packetlens {
    enum class NetworkProtocol {
        Unknown,
        IPv4,
        IPv6,
        ARP
    };

    enum class TransportProtocol {
        None,
        TCP,
        UDP,
        ICMP,
        Unknown
    };

    class EthernetInfo{
    public:
        std::array<std::uint8_t, 6> destination;
        std::array<std::uint8_t, 6> source;
        std::uint16_t ether_type;
    };

    class IPv4Info{
    public:
        std::array<std::uint8_t, 4> source;
        std::array<std::uint8_t, 4> destination;

        std::uint8_t ttl;
        std::uint8_t protocol;

        std::uint8_t header_length;
        std::uint16_t total_length;
    };

    class TcpInfo {
    public:
        std::uint16_t source_port;
        std::uint16_t destination_port;

        std::uint32_t sequence_number;
        std::uint32_t acknowledgment_number;

        std::uint8_t header_length;
    };

    class UdpInfo{
    public:
        std::uint16_t source_port;
        std::uint16_t destination_port;
        std::uint16_t length;
    };

    class DecodedPacket{
    public:
        std::optional<EthernetInfo> ethernet;
        std::optional<IPv4Info> ipv4;

        std::optional<TcpInfo> tcp;
        std::optional<UdpInfo> udp;

        NetworkProtocol network_protocol{NetworkProtocol::Unknown};
        TransportProtocol transport_protocol{TransportProtocol::None};

        bool valid{false};
    };
}

#endif