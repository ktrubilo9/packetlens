#ifndef PCAP_SOURCE_HPP
#define PCAP_SOURCE_HPP

#include <packetlens/capture/packet_source.hpp>
#include <fstream>
#include <string>
#include <string_view>
#include <cstdint>

namespace packetlens {
    class PcapSource : public PacketSource {
        enum class ByteOrder { LittleEndian, BigEndian };
        enum class TimestampPrecision { Microseconds, Nanoseconds };

        std::string file_name_;
        std::ifstream file_;

        ByteOrder byte_order_{ByteOrder::LittleEndian};
        TimestampPrecision timestamp_precision_{TimestampPrecision::Microseconds};
        std::uint32_t snaplen_{0};

        void parse_magic(const std::uint8_t* data);
        std::uint16_t read_u16(const std::uint8_t* data, std::size_t offset) const;
        std::uint32_t read_u32(const std::uint8_t* data, std::size_t offset) const;
    public:
        explicit PcapSource(std::string_view file_name);

        PcapSource(const PcapSource&) = delete;
        PcapSource& operator=(const PcapSource&) = delete;
        PcapSource(PcapSource&& other) = default;
        PcapSource& operator=(PcapSource&& other) = default;

        void open() override;
        /// Reads captured Ethernet bytes unchanged, with the timestamp from the file.
        /// Returns false only at a record boundary at EOF. Frames up to 65536 bytes
        /// are supported. On malformed input or I/O failure, closes the source and
        /// throws, leaving frame unchanged; open() can restart from the beginning.
        bool receive(RawFrame& frame) override;
        void close() noexcept override;

        ~PcapSource() override;
    };
}

#endif
