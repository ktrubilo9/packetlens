#include <packetlens/pcap_source.hpp>
#include <packetlens/decoder.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unistd.h>

namespace {
using packetlens::PcapSource;
using packetlens::RawFrame;
using Bytes = std::vector<std::uint8_t>;

void append(Bytes& bytes, std::uint32_t value, unsigned width, bool little) {
    for (unsigned i = 0; i < width; ++i) {
        const auto shift = 8 * (little ? i : width - 1 - i);
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

Bytes file_header(bool little = true, bool nanos = false, std::uint32_t snaplen = 65535) {
    Bytes bytes;
    append(bytes, nanos ? 0xa1b23c4d : 0xa1b2c3d4, 4, little);
    append(bytes, 2, 2, little);
    append(bytes, 4, 2, little);
    append(bytes, 0, 4, little);
    append(bytes, 0, 4, little);
    append(bytes, snaplen, 4, little);
    append(bytes, 1, 4, little);
    return bytes;
}

void record(Bytes& bytes, const Bytes& payload, bool little = true,
            std::uint32_t seconds = 1700000000, std::uint32_t fraction = 123456,
            std::uint32_t original = 60) {
    append(bytes, seconds, 4, little);
    append(bytes, fraction, 4, little);
    append(bytes, static_cast<std::uint32_t>(payload.size()), 4, little);
    append(bytes, original, 4, little);
    bytes.insert(bytes.end(), payload.begin(), payload.end());
}

class TempPcap {
public:
    std::string path;
    explicit TempPcap(const Bytes& bytes) {
        auto pattern = (std::filesystem::temp_directory_path() / "packetlens-pcap-XXXXXX").string();
        const int fd = ::mkstemp(pattern.data());
        if (fd == -1) { throw std::runtime_error("cannot create temporary PCAP"); }
        ::close(fd);
        path = pattern;
        std::ofstream output(path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        output.close();
        if (!output) {
            std::filesystem::remove(path);
            throw std::runtime_error("cannot write temporary PCAP");
        }
    }
    ~TempPcap() {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
    TempPcap(const TempPcap&) = delete;
    TempPcap& operator=(const TempPcap&) = delete;
};

void expect_bad_record(const Bytes& bytes) {
    TempPcap file(bytes);
    PcapSource source(file.path);
    source.open();
    RawFrame frame;
    frame.data = {0xaa, 0xbb};
    frame.timestamp = std::chrono::system_clock::time_point{std::chrono::seconds{42}};
    const auto previous = frame;
    CHECK_THROWS_AS(source.receive(frame), std::runtime_error);
    CHECK(frame.data == previous.data);
    CHECK(frame.timestamp == previous.timestamp);
    CHECK_THROWS_AS(source.receive(frame), std::logic_error);
}
}

TEST_CASE("PCAP preserves packet bytes in all byte orders and timestamp precisions", "[pcap]") {
    for (const bool little : {false, true}) {
        for (const bool nanos : {false, true}) {
            CAPTURE(little, nanos);
            auto bytes = file_header(little, nanos);
            const Bytes payload{0, 0x80, 0xff, 0, 0x12};
            const auto fraction = nanos ? 123456789u : 123456u;
            record(bytes, payload, little, 1700000000, fraction);
            record(bytes, {0x42}, little, 1700000001, 0);
            TempPcap file(bytes);
            PcapSource source(file.path);
            source.open();
            RawFrame frame;
            REQUIRE(source.receive(frame));
            CHECK(frame.data == payload);
            const auto expected = std::chrono::seconds{1700000000}
                + std::chrono::nanoseconds{nanos ? 123456789 : 123456000};
            CHECK(frame.timestamp.time_since_epoch() == expected);
            REQUIRE(source.receive(frame)); // No alignment padding between records.
            CHECK(frame.data == Bytes{0x42});
            CHECK(frame.timestamp.time_since_epoch() == std::chrono::seconds{1700000001});
            const auto previous = frame;
            CHECK_FALSE(source.receive(frame));
            CHECK_FALSE(source.receive(frame));
            CHECK(frame.data == previous.data);
            CHECK(frame.timestamp == previous.timestamp);
        }
    }
}

TEST_CASE("PCAP header-only file has clean EOF and can reopen", "[pcap]") {
    TempPcap file(file_header());
    PcapSource source(file.path);
    RawFrame frame;
    CHECK_THROWS_AS(source.receive(frame), std::logic_error);
    source.open();
    CHECK_THROWS_AS(source.open(), std::logic_error);
    CHECK_FALSE(source.receive(frame));
    source.close();
    CHECK_THROWS_AS(source.receive(frame), std::logic_error);
    source.open();
    CHECK_FALSE(source.receive(frame));
}

TEST_CASE("PCAP zero-length records are not EOF", "[pcap]") {
    auto bytes = file_header();
    record(bytes, {}, true, 1, 0, 0);
    record(bytes, {0xff});
    TempPcap file(bytes);
    PcapSource source(file.path);
    source.open();
    RawFrame frame;
    frame.data = {1};
    REQUIRE(source.receive(frame));
    CHECK(frame.data.empty());
    REQUIRE(source.receive(frame));
    CHECK(frame.data == Bytes{0xff});
    CHECK_FALSE(source.receive(frame));
}

TEST_CASE("PCAP rejects every partial record header", "[pcap]") {
    for (std::size_t length = 1; length < 16; ++length) {
        CAPTURE(length);
        auto bytes = file_header();
        bytes.resize(24 + length, 0);
        expect_bad_record(bytes);
    }
}

TEST_CASE("PCAP rejects truncated packet data", "[pcap]") {
    auto bytes = file_header();
    record(bytes, {1, 2, 3});
    bytes.pop_back();
    expect_bad_record(bytes);
}

TEST_CASE("PCAP validates record lengths before allocating", "[pcap]") {
    auto bytes = file_header();
    SECTION("captured length exceeds snaplen") {
        bytes = file_header(true, false, 2);
        record(bytes, {1, 2, 3});
    }
    SECTION("captured length exceeds original length") {
        record(bytes, {1, 2, 3}, true, 1, 0, 2);
    }
    SECTION("huge length cannot cause a huge allocation") {
        bytes = file_header(true, false, 0xffffffff);
        append(bytes, 1, 4, true);
        append(bytes, 0, 4, true);
        append(bytes, 0xffffffff, 4, true);
        append(bytes, 0xffffffff, 4, true);
    }
    expect_bad_record(bytes);
}

TEST_CASE("PCAP accepts the supported maximum frame size", "[pcap]") {
    auto bytes = file_header(true, false, 65536);
    const Bytes payload(65536, 0xa5);
    record(bytes, payload, true, 1, 0, 65536);
    TempPcap file(bytes);
    PcapSource source(file.path);
    source.open();
    RawFrame frame;
    REQUIRE(source.receive(frame));
    CHECK(frame.data == payload);
}

TEST_CASE("PCAP rejects out-of-range timestamp fractions", "[pcap]") {
    for (const bool nanos : {false, true}) {
        auto bytes = file_header(true, nanos);
        record(bytes, {1}, true, 1, nanos ? 1000000000 : 1000000);
        expect_bad_record(bytes);
    }
}

TEST_CASE("PCAP timestamps support seconds beyond signed 32-bit range", "[pcap]") {
    auto bytes = file_header(true, true);
    record(bytes, {1}, true, 0xffffffff, 999999999);
    TempPcap file(bytes);
    PcapSource source(file.path);
    source.open();
    RawFrame frame;
    REQUIRE(source.receive(frame));
    CHECK(frame.timestamp.time_since_epoch() ==
          std::chrono::seconds{4294967295LL} + std::chrono::nanoseconds{999999999});
}

TEST_CASE("PCAP rejects unsupported versions and zero snaplen", "[pcap]") {
    auto bytes = file_header();
    SECTION("old version") { bytes[6] = 3; }
    SECTION("unknown minor version") { bytes[6] = 5; }
    SECTION("zero snaplen") { bytes = file_header(true, false, 0); }
    TempPcap file(bytes);
    PcapSource source(file.path);
    CHECK_THROWS_AS(source.open(), std::runtime_error);
    RawFrame frame;
    CHECK_THROWS_AS(source.receive(frame), std::logic_error);
}

TEST_CASE("PCAP Ethernet data can be passed directly to Decoder", "[pcap]") {
    // Ethernet + IPv4 + UDP, with source port 40000 and destination port 9000.
    const Bytes payload{
        2,0,0,0,0,2, 2,0,0,0,0,1, 0x08,0x00,
        0x45,0,0,28, 0,1,0x40,0, 64,17,0,0,
        192,0,2,1, 198,51,100,2,
        0x9c,0x40,0x23,0x28, 0,8,0,0
    };
    auto bytes = file_header();
    record(bytes, payload);
    TempPcap file(bytes);
    PcapSource source(file.path);
    source.open();
    RawFrame frame;
    REQUIRE(source.receive(frame));
    const auto packet = packetlens::Decoder{}.decode(frame);
    REQUIRE(packet.udp);
    CHECK(packet.udp->source_port == 40000);
    CHECK(packet.udp->destination_port == 9000);
    CHECK(packet.udp->length == 8);
}
