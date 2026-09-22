#include <packetlens/decode/decoder.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>

namespace {
using namespace packetlens;

void write16(RawFrame& frame, std::size_t offset, std::uint16_t value) {
    frame.data.at(offset) = static_cast<std::uint8_t>(value >> 8);
    frame.data.at(offset + 1) = static_cast<std::uint8_t>(value);
}

RawFrame ethernet(std::uint16_t type = 0x0800) {
    RawFrame frame;
    frame.data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 0};
    write16(frame, 12, type);
    return frame;
}

RawFrame ipv4(std::uint8_t protocol, std::size_t payload_size,
              std::size_t header_size = 20) {
    auto frame = ethernet();
    frame.data.resize(14 + header_size + payload_size);
    frame.data[14] = static_cast<std::uint8_t>(0x40 | (header_size / 4));
    write16(frame, 16, static_cast<std::uint16_t>(header_size + payload_size));
    frame.data[22] = 64;
    frame.data[23] = protocol;
    frame.data[26] = 192; frame.data[27] = 0; frame.data[28] = 2; frame.data[29] = 1;
    frame.data[30] = 198; frame.data[31] = 51; frame.data[32] = 100; frame.data[33] = 2;
    return frame;
}

RawFrame tcp(std::size_t ip_header_size = 20, std::size_t tcp_header_size = 20) {
    auto frame = ipv4(6, tcp_header_size, ip_header_size);
    const auto start = 14 + ip_header_size;
    write16(frame, start, 0xc001);
    write16(frame, start + 2, 443);
    // Distinct high bytes exercise unsigned, big-endian 32-bit reads.
    frame.data[start + 4] = 0x89; frame.data[start + 5] = 0xab;
    frame.data[start + 6] = 0xcd; frame.data[start + 7] = 0xef;
    frame.data[start + 8] = 0xfe; frame.data[start + 9] = 0xdc;
    frame.data[start + 10] = 0xba; frame.data[start + 11] = 0x98;
    frame.data[start + 12] = static_cast<std::uint8_t>((tcp_header_size / 4) << 4);
    return frame;
}

RawFrame udp(std::size_t payload_size = 4) {
    auto frame = ipv4(17, 8 + payload_size);
    write16(frame, 34, 5353);
    write16(frame, 36, 0xabcd);
    write16(frame, 38, static_cast<std::uint16_t>(8 + payload_size));
    return frame;
}

DecodedPacket decode(const RawFrame& frame) { return Decoder{}.decode(frame); }
}

TEST_CASE("Decoder rejects every truncated Ethernet header", "[decoder][ethernet]") {
    for (std::size_t length = 0; length < 14; ++length) {
        CAPTURE(length);
        auto frame = ethernet();
        frame.data.resize(length);
        const auto packet = decode(frame);
        CHECK_FALSE(packet.valid);
        CHECK_FALSE(packet.ethernet);
        CHECK_FALSE(packet.ipv4);
        CHECK_FALSE(packet.tcp);
        CHECK_FALSE(packet.udp);
        CHECK(packet.network_protocol == NetworkProtocol::Unknown);
        CHECK(packet.transport_protocol == TransportProtocol::None);
    }
}

TEST_CASE("Decoder reads Ethernet addresses and classifies EtherTypes", "[decoder][ethernet]") {
    for (const auto type : {0x0800, 0x86dd, 0x0806, 0x88b5}) {
        CAPTURE(type);
        const auto packet = decode(ethernet(static_cast<std::uint16_t>(type)));
        REQUIRE(packet.valid);
        REQUIRE(packet.ethernet);
        CHECK(packet.ethernet->destination == (std::array<std::uint8_t, 6>{0, 1, 2, 3, 4, 5}));
        CHECK(packet.ethernet->source == (std::array<std::uint8_t, 6>{6, 7, 8, 9, 10, 11}));
        CHECK(packet.ethernet->ether_type == type);
        const auto expected = type == 0x0800 ? NetworkProtocol::IPv4 :
                              type == 0x86dd ? NetworkProtocol::IPv6 :
                              type == 0x0806 ? NetworkProtocol::ARP : NetworkProtocol::Unknown;
        CHECK(packet.network_protocol == expected);
        CHECK_FALSE(packet.ipv4);
        CHECK_FALSE(packet.tcp);
        CHECK_FALSE(packet.udp);
    }
}

TEST_CASE("Decoder reads IPv4 and TCP fields in network byte order", "[decoder][ipv4][tcp]") {
    const auto packet = decode(tcp());
    REQUIRE(packet.ipv4);
    CHECK(packet.ipv4->source == (std::array<std::uint8_t, 4>{192, 0, 2, 1}));
    CHECK(packet.ipv4->destination == (std::array<std::uint8_t, 4>{198, 51, 100, 2}));
    CHECK(packet.ipv4->ttl == 64);
    CHECK(packet.ipv4->protocol == 6);
    CHECK(packet.ipv4->header_length == 20);
    CHECK(packet.ipv4->total_length == 40);
    REQUIRE(packet.tcp);
    CHECK(packet.transport_protocol == TransportProtocol::TCP);
    CHECK(packet.tcp->source_port == 49153);
    CHECK(packet.tcp->destination_port == 443);
    CHECK(packet.tcp->sequence_number == 0x89abcdefu);
    CHECK(packet.tcp->acknowledgment_number == 0xfedcba98u);
    CHECK(packet.tcp->header_length == 20);
    CHECK_FALSE(packet.udp);
}

TEST_CASE("Decoder respects IPv4 and TCP options", "[decoder][ipv4][tcp]") {
    for (const auto ip_length : {20u, 24u, 60u}) {
        for (const auto tcp_length : {20u, 24u, 60u}) {
            CAPTURE(ip_length, tcp_length);
            const auto packet = decode(tcp(ip_length, tcp_length));
            REQUIRE(packet.ipv4);
            CHECK(packet.ipv4->header_length == ip_length);
            REQUIRE(packet.tcp);
            CHECK(packet.tcp->header_length == tcp_length);
            CHECK(packet.tcp->source_port == 49153);
            CHECK(packet.tcp->sequence_number == 0x89abcdefu);
        }
    }
}

TEST_CASE("Decoder rejects truncated IPv4 headers", "[decoder][ipv4]") {
    for (std::size_t length = 0; length < 20; ++length) {
        CAPTURE(length);
        auto frame = tcp();
        frame.data.resize(14 + length);
        const auto packet = decode(frame);
        CHECK(packet.valid); // valid describes only the outer Ethernet header.
        CHECK_FALSE(packet.ipv4);
        CHECK_FALSE(packet.tcp);
    }
}

TEST_CASE("Decoder rejects malformed IPv4 lengths and versions", "[decoder][ipv4]") {
    auto frame = tcp();
    SECTION("wrong version") { frame.data[14] = 0x65; }
    SECTION("IHL below five") { frame.data[14] = 0x44; }
    SECTION("IHL beyond captured bytes") { frame.data[14] = 0x4f; }
    SECTION("total length below header length") { write16(frame, 16, 19); }
    SECTION("total length exceeds captured bytes") { write16(frame, 16, 41); }
    const auto packet = decode(frame);
    CHECK(packet.valid);
    CHECK_FALSE(packet.ipv4);
    CHECK_FALSE(packet.tcp);
    CHECK_FALSE(packet.udp);
}

TEST_CASE("Decoder rejects truncated TCP headers", "[decoder][tcp]") {
    for (std::size_t length = 0; length < 20; ++length) {
        CAPTURE(length);
        const auto packet = decode(ipv4(6, length));
        REQUIRE(packet.ipv4);
        CHECK_FALSE(packet.tcp);
        CHECK(packet.transport_protocol == TransportProtocol::None);
    }
}

TEST_CASE("Decoder rejects invalid TCP data offsets", "[decoder][tcp]") {
    for (const auto offset : {0x00, 0x40, 0x60, 0xf0}) {
        CAPTURE(offset);
        auto frame = tcp();
        frame.data[46] = static_cast<std::uint8_t>(offset);
        const auto packet = decode(frame);
        REQUIRE(packet.ipv4);
        CHECK_FALSE(packet.tcp);
    }
}

TEST_CASE("Decoder reads UDP headers with and without payload", "[decoder][udp]") {
    for (const auto payload_length : {0u, 4u, 64u}) {
        CAPTURE(payload_length);
        const auto packet = decode(udp(payload_length));
        REQUIRE(packet.udp);
        CHECK(packet.transport_protocol == TransportProtocol::UDP);
        CHECK(packet.udp->source_port == 5353);
        CHECK(packet.udp->destination_port == 43981);
        CHECK(packet.udp->length == 8 + payload_length);
        CHECK_FALSE(packet.tcp);
    }
}

TEST_CASE("Decoder rejects truncated UDP headers", "[decoder][udp]") {
    for (std::size_t length = 0; length < 8; ++length) {
        CAPTURE(length);
        const auto packet = decode(ipv4(17, length));
        REQUIRE(packet.ipv4);
        CHECK_FALSE(packet.udp);
        CHECK(packet.transport_protocol == TransportProtocol::None);
    }
}

TEST_CASE("Decoder rejects invalid UDP lengths", "[decoder][udp]") {
    for (const auto length : {0, 7, 13, 65535}) {
        CAPTURE(length);
        auto frame = udp();
        write16(frame, 38, static_cast<std::uint16_t>(length));
        CHECK_FALSE(decode(frame).udp);
    }
}

TEST_CASE("Ethernet padding cannot supply missing transport bytes", "[decoder][ipv4]") {
    SECTION("TCP header outside IPv4 total length") {
        auto frame = tcp();
        write16(frame, 16, 20);
        CHECK_FALSE(decode(frame).tcp);
    }
    SECTION("UDP payload outside IPv4 total length") {
        auto frame = udp();
        write16(frame, 16, 28);
        CHECK_FALSE(decode(frame).udp);
    }
    SECTION("padding after a complete datagram is ignored") {
        auto frame = udp();
        frame.data.resize(100, 0xff);
        const auto packet = decode(frame);
        REQUIRE(packet.udp);
        CHECK(packet.udp->length == 12);
    }
}

TEST_CASE("Decoder does not interpret non-initial fragments as transport headers", "[decoder][ipv4]") {
    for (const auto protocol : {6, 17}) {
        for (const auto flags_offset : {0x0001, 0x2001, 0x1fff}) {
            CAPTURE(protocol, flags_offset);
            auto frame = protocol == 6 ? tcp() : udp();
            write16(frame, 20, static_cast<std::uint16_t>(flags_offset));
            const auto packet = decode(frame);
            REQUIRE(packet.ipv4);
            CHECK_FALSE(packet.tcp);
            CHECK_FALSE(packet.udp);
            CHECK(packet.transport_protocol == TransportProtocol::None);
        }
    }
}

TEST_CASE("Decoder reads transport headers from initial IPv4 fragments", "[decoder][ipv4]") {
    SECTION("TCP first fragment containing a complete header") {
        auto frame = tcp();
        frame.data.resize(14 + 20 + 24);
        write16(frame, 16, 44);
        write16(frame, 20, 0x2000);
        REQUIRE(decode(frame).tcp);
    }
    SECTION("UDP length describes the whole datagram rather than this fragment") {
        auto frame = udp(8); // 16 fragment payload bytes, a multiple of eight.
        write16(frame, 20, 0x2000);
        write16(frame, 38, 32);
        const auto packet = decode(frame);
        REQUIRE(packet.udp);
        CHECK(packet.udp->length == 32);
        CHECK(packet.transport_protocol == TransportProtocol::UDP);
    }
    SECTION("MF does not make an undersized UDP length valid") {
        auto frame = udp(8);
        write16(frame, 20, 0x2000);
        write16(frame, 38, 7);
        CHECK_FALSE(decode(frame).udp);
    }
    SECTION("DF flag does not prevent ordinary decoding") {
        auto frame = udp();
        write16(frame, 20, 0x4000);
        REQUIRE(decode(frame).udp);
    }
}

TEST_CASE("Decoder classifies ICMP and unknown IPv4 protocols", "[decoder][ipv4]") {
    CHECK(decode(ipv4(1, 8)).transport_protocol == TransportProtocol::ICMP);
    CHECK(decode(ipv4(253, 8)).transport_protocol == TransportProtocol::Unknown);
}

TEST_CASE("Decoder has no state between calls and does not mutate input", "[decoder]") {
    Decoder decoder;
    const auto frame = tcp();
    const auto original = frame.data;
    REQUIRE(decoder.decode(frame).tcp);
    CHECK(frame.data == original);
    const auto packet = decoder.decode(RawFrame{});
    CHECK_FALSE(packet.valid);
    CHECK_FALSE(packet.ethernet);
    CHECK_FALSE(packet.ipv4);
    CHECK_FALSE(packet.tcp);
    CHECK_FALSE(packet.udp);
}
