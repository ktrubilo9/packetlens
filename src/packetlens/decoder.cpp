#include <packetlens/decoder.hpp>
#include <packetlens/byte_utils.hpp>

using namespace packetlens;

bool Decoder::decode_ethernet(const std::uint8_t* data, std::size_t size, DecodedPacket& result) const {
    if (size < kEthernetHeaderSize) {
        return false;
    }

    EthernetInfo ethernet{};
    for (std::size_t i=0; i < kEthernetAddressSize; i++) {
        ethernet.destination[i] = data[i];
        ethernet.source[i] = data[i + kEthernetAddressSize];
    }

    ethernet.ether_type = read_u16_be(data, 12);
    result.ethernet = ethernet;

    switch (ethernet.ether_type) {
        case kEtherTypeIpv4:
            result.network_protocol = NetworkProtocol::IPv4;
            break;
        case kEtherTypeIpv6:
            result.network_protocol = NetworkProtocol::IPv6;
            break;
        case kEtherTypeArp:
            result.network_protocol = NetworkProtocol::ARP;
            break;
        default:
            result.network_protocol = NetworkProtocol::Unknown;
            break;
    }

    return true;
}

void Decoder::decode_ipv4(const std::uint8_t* data, std::size_t size, DecodedPacket& result) const {
    if (size < kMinimumIpv4HeaderSize) {
        return;
    }

    const std::uint8_t version_ihl = data[0];

    const std::uint8_t version = version_ihl >> 4;
    const std::uint8_t ihl = version_ihl & 0x0f;

    // ipv4 requires version 4 and ihl of at least 5
    if (version != 4 || ihl < 5) {
        return;
    }

    const std::size_t header_length = static_cast<std::size_t>(ihl) * 4;
    if (header_length > size) {
        return;
    }

    const std::uint16_t total_length = read_u16_be(data, 2);

    if (total_length < header_length || total_length > size) {
        return;
    } 

    IPv4Info ipv4{};

    ipv4.header_length = static_cast<std::uint8_t>(header_length);
    ipv4.total_length = total_length;
    ipv4.ttl = data[8];
    ipv4.protocol = data[9];

    for (std::size_t i=0; i< 4; i++) {
        ipv4.source[i] = data[12 + i];
        ipv4.destination[i] = data[16 + i];
    }

    result.ipv4 = ipv4;

    const std::uint16_t fragmentation = read_u16_be(data, 6);
    const std::uint16_t fragment_offset = fragmentation & 0x1fff;
    const bool more_fragments = (fragmentation & 0x2000) != 0;

    if (fragment_offset != 0) {
        return;
    }

    const auto* transport_data = data + header_length;
    const std::size_t transport_size = static_cast<std::size_t>(total_length) - header_length;

    switch(ipv4.protocol) {
        case kIpProtocolTcp:
            if (transport_size < kMinimumTcpHeaderSize) {
                return;
            }
            {
                TcpInfo tcp{};
                tcp.source_port = read_u16_be(transport_data, 0);
                tcp.destination_port = read_u16_be(transport_data, 2);
                tcp.sequence_number = read_u32_be(transport_data, 4);
                tcp.acknowledgment_number = read_u32_be(transport_data, 8);
                const std::uint8_t data_offset = transport_data[12] >> 4;
                const std::size_t tcp_header_length = static_cast<std::size_t>(data_offset) * 4;

                // TCP data offset has the same minimum of 5
                // 32-bit words (20 bytes)
                if (tcp_header_length < kMinimumIpv4HeaderSize || tcp_header_length > transport_size) {
                    return;
                }

                tcp.header_length = static_cast<std::uint8_t>(tcp_header_length);
                result.tcp = tcp;
                result.transport_protocol = TransportProtocol::TCP;
            }
            break;
        case kIpProtocolUdp:
            if (transport_size < kUdpHeaderSize) {
                return;
            }

            {
                UdpInfo udp{};

                udp.source_port =
                    read_u16_be(transport_data, 0);

                udp.destination_port =
                    read_u16_be(transport_data, 2);

                udp.length =
                    read_u16_be(transport_data, 4);

                // In the first fragment, UDP length includes subsequent fragments.
                if (udp.length < kUdpHeaderSize ||
                    (!more_fragments && udp.length > transport_size)) {
                    return;
                }

                result.udp = udp;
                result.transport_protocol =
                    TransportProtocol::UDP;
            }

            break;

        case kIpProtocolIcmp:
            result.transport_protocol =
                TransportProtocol::ICMP;
            break;

        default:
            result.transport_protocol =
                TransportProtocol::Unknown;
            break;
    }
}

DecodedPacket Decoder::decode(const RawFrame& frame) const {
    DecodedPacket result;

    // RawFrame owns the bytes, decoder only reads
    const uint8_t* data = frame.data.data();
    const std::size_t size = frame.data.size();

    if (!decode_ethernet(data, size, result)) {
        return result;
    }

    // valid means that the outer Ethernet frame is structurally valid
    result.valid = true;

    switch (result.network_protocol) {
        case NetworkProtocol::IPv4: {
            const uint8_t* ipv4_data = data + kEthernetHeaderSize;
            const std::size_t ipv4_size = size - kEthernetHeaderSize;

            decode_ipv4(ipv4_data, ipv4_size, result);

            break;
        }
        case NetworkProtocol::IPv6:
            // todo
            break;
        case NetworkProtocol::ARP:
            // todo    
            break;
        case NetworkProtocol::Unknown:
            // valid Ethernet frame with an unsupported EtherType
            break;
        default:
            break;
    }
    return result;
}
