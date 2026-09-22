#include <packetlens/output/text_formatter.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>

using namespace packetlens;

TEST_CASE("Formatter prints UDP endpoints and payload length in UTC", "[formatter]") {
    RawFrame frame;
    frame.timestamp = std::chrono::system_clock::time_point{
        std::chrono::seconds{1700000000} + std::chrono::microseconds{100000}};
    DecodedPacket packet;
    packet.ethernet = EthernetInfo{};
    packet.ipv4 = IPv4Info{};
    packet.ipv4->source = {192, 0, 2, 1};
    packet.ipv4->destination = {198, 51, 100, 2};
    packet.udp = UdpInfo{40001, 9000, 25};
    CHECK(TextFormatter::format(frame, packet) ==
          "22:13:20.100000 IP 192.0.2.1.40001 > 198.51.100.2.9000: UDP, length 17");
}

TEST_CASE("Formatter prints TCP unsigned sequence numbers", "[formatter]") {
    RawFrame frame;
    frame.timestamp = std::chrono::system_clock::time_point{};
    DecodedPacket packet;
    packet.ethernet = EthernetInfo{};
    packet.ipv4 = IPv4Info{};
    packet.ipv4->source = {192, 0, 2, 1};
    packet.ipv4->destination = {198, 51, 100, 2};
    packet.tcp = TcpInfo{49153, 443, 0xffffffffu, 0x80000000u, 20};
    CHECK(TextFormatter::format(frame, packet) ==
          "00:00:00.000000 IP 192.0.2.1.49153 > 198.51.100.2.443: TCP, seq 4294967295, ack 2147483648");
}

TEST_CASE("Formatter handles missing layers without assuming packet validity", "[formatter]") {
    RawFrame frame;
    frame.timestamp = std::chrono::system_clock::time_point{};
    frame.data.resize(10);
    DecodedPacket packet;
    CHECK(TextFormatter::format(frame, packet) ==
          "00:00:00.000000 [incomplete Ethernet header], captured 10 bytes");
    packet.ethernet = EthernetInfo{};
    packet.ethernet->ether_type = 0x0806;
    CHECK(TextFormatter::format(frame, packet) ==
          "00:00:00.000000 Ethernet, ethertype 0x0806, captured 10 bytes");
    packet.ipv4 = IPv4Info{};
    packet.ipv4->source = {192, 0, 2, 1};
    packet.ipv4->destination = {198, 51, 100, 2};
    packet.ipv4->protocol = 1;
    packet.ipv4->total_length = 28;
    CHECK(TextFormatter::format(frame, packet) ==
          "00:00:00.000000 IP 192.0.2.1 > 198.51.100.2: proto 1, length 28");
}
