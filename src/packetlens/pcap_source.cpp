#include <packetlens/pcap_source.hpp>
#include <packetlens/byte_utils.hpp>
#include <stdexcept>
#include <array>
#include <chrono>
#include <utility>

using packetlens::PcapSource;

namespace {
    constexpr std::uint16_t SUPPORTED_MAJOR = 2;
    constexpr std::uint16_t SUPPORTED_MINOR = 4;
    // Match the maximum frame buffer currently used by SocketSource.
    constexpr std::uint32_t MAX_CAPTURED_LENGTH = 65536;
    constexpr std::uint32_t LINKTYPE_ETHERNET = 1;
    constexpr std::uint32_t LINKTYPE_MASK = 0x03FFFFFF;
}

PcapSource::PcapSource(std::string_view file_name) 
    :file_name_(file_name) {

}

void PcapSource::parse_magic(const std::uint8_t* data) {
    if (data[0] == 0xd4 && 
        data[1] == 0xc3 && 
        data[2] == 0xb2 && 
        data[3] == 0xa1) {
            byte_order_ = ByteOrder::LittleEndian;
            timestamp_precision_ = TimestampPrecision::Microseconds;
            return;
    }
    if (data[0] == 0xa1 &&
        data[1] == 0xb2 &&
        data[2] == 0xc3 &&
        data[3] == 0xd4) {

            byte_order_ = ByteOrder::BigEndian;
            timestamp_precision_ = TimestampPrecision::Microseconds;
            return;
    }

    if (data[0] == 0x4d &&
        data[1] == 0x3c &&
        data[2] == 0xb2 &&
        data[3] == 0xa1) {

            byte_order_ = ByteOrder::LittleEndian;
            timestamp_precision_ = TimestampPrecision::Nanoseconds;
            return;
    }

    if (data[0] == 0xa1 &&
        data[1] == 0xb2 &&
        data[2] == 0x3c &&
        data[3] == 0x4d) {
            byte_order_ = ByteOrder::BigEndian;
            timestamp_precision_ = TimestampPrecision::Nanoseconds;
            return;
    }

    throw std::runtime_error("invalid PCAP magic number");
}

std::uint16_t PcapSource::read_u16(
    const std::uint8_t* data,
    std::size_t offset
) const {
    return byte_order_ == ByteOrder::LittleEndian
        ? read_u16_le(data, offset)
        : read_u16_be(data, offset);
}

std::uint32_t PcapSource::read_u32(
    const std::uint8_t* data,
    std::size_t offset
) const {
    return byte_order_ == ByteOrder::LittleEndian
        ? read_u32_le(data, offset)
        : read_u32_be(data, offset);
}

bool PcapSource::receive(RawFrame& frame) {
    if (!file_.is_open()) {
        throw std::logic_error("pcap source is not open");
    }

    try {
        std::array<std::uint8_t, 16> header{};
        file_.read(reinterpret_cast<char*>(header.data()), header.size());
        if (file_.bad()) {
            throw std::runtime_error("failed to read PCAP record header");
        }
        if (file_.gcount() == 0 && file_.eof()) {
            return false;
        }
        if (file_.gcount() != static_cast<std::streamsize>(header.size())) {
            throw std::runtime_error("truncated PCAP record header");
        }

        const auto seconds = read_u32(header.data(), 0);
        const auto fraction = read_u32(header.data(), 4);
        const auto captured_length = read_u32(header.data(), 8);
        const auto original_length = read_u32(header.data(), 12);
        const bool microseconds = timestamp_precision_ == TimestampPrecision::Microseconds;
        if (fraction >= (microseconds ? 1000000u : 1000000000u)) {
            throw std::runtime_error("invalid PCAP timestamp fraction");
        }
        if (captured_length > snaplen_ || captured_length > original_length) {
            throw std::runtime_error("invalid PCAP captured length");
        }
        if (captured_length > MAX_CAPTURED_LENGTH) {
            throw std::runtime_error("PCAP captured length exceeds supported limit of 65536 bytes");
        }

        RawFrame next;
        next.data.resize(captured_length);
        if (captured_length != 0) {
            file_.read(reinterpret_cast<char*>(next.data.data()), captured_length);
            if (file_.bad()) {
                throw std::runtime_error("failed to read PCAP packet data");
            }
            if (file_.gcount() != static_cast<std::streamsize>(captured_length)) {
                throw std::runtime_error("truncated PCAP packet data");
            }
        }

        // Both PCAP fields are unsigned 32-bit, widen before converting to nanoseconds.
        const auto elapsed = std::chrono::seconds{static_cast<std::int64_t>(seconds)}
            + std::chrono::nanoseconds{static_cast<std::int64_t>(fraction)
                                      * (microseconds ? 1000 : 1)};
        next.timestamp = std::chrono::system_clock::time_point{
            std::chrono::duration_cast<std::chrono::system_clock::duration>(elapsed)};
        frame = std::move(next);
        return true;
    } catch (...) {
        // A partial/invalid record leaves no reliable next-record boundary.
        close();
        throw;
    }
}

void PcapSource::open() {
    if (file_.is_open()) {
        throw std::logic_error("pcap source is already open");
    }
    if (file_name_.empty()) {
        throw std::runtime_error("pcap file name is empty");
    }

    file_.open(file_name_, std::ios::binary);
    if (!file_) {
        close();
        throw std::runtime_error("failed to open file: " + file_name_);
    }

    try {
        std::array<std::uint8_t, 24> header{};
        if (!file_.read(reinterpret_cast<char*>(header.data()),
                        static_cast<std::streamsize>(header.size()))) {
            throw std::runtime_error("failed to read PCAP header");
        }

        parse_magic(header.data());

        const auto major = read_u16(header.data(), 4);
        const auto minor = read_u16(header.data(), 6);
        if (major != SUPPORTED_MAJOR || minor != SUPPORTED_MINOR) {
            throw std::runtime_error(
                "unsupported PCAP version: " + std::to_string(major) +
                "." + std::to_string(minor));
        }

        snaplen_ = read_u32(header.data(), 16);
        if (snaplen_ == 0) {
            throw std::runtime_error("invalid PCAP snaplen: 0");
        }

        const auto linktype = read_u32(header.data(), 20) & LINKTYPE_MASK;
        if (linktype != LINKTYPE_ETHERNET) {
            throw std::runtime_error(
                "unsupported PCAP link type: " + std::to_string(linktype));
        }
    } catch (...) {
        close();
        throw;
    }
}

void PcapSource::close() noexcept {
    if (file_.is_open()) {
        file_.close();
    }

    file_.clear();

    byte_order_ = ByteOrder::LittleEndian;
    timestamp_precision_ = TimestampPrecision::Microseconds;
    snaplen_ = 0;
}

PcapSource::~PcapSource() {
    close();
}
