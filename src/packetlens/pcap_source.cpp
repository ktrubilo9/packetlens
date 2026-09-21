#include <packetlens/pcap_source.hpp>
#include <packetlens/byte_utils.hpp>
#include <stdexcept>
#include <array>

using packetlens::PcapSource;

namespace {
    constexpr std::uint16_t SUPPORTED_MAJOR = 2;
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
    return true;
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
        if (major != SUPPORTED_MAJOR) {
            throw std::runtime_error(
                "unsupported PCAP version: " + std::to_string(major) +
                "." + std::to_string(minor));
        }

        snaplen_ = read_u32(header.data(), 16);

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