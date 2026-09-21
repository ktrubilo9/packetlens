#ifndef PCAP_SOURCE_HPP
#define PCAP_SOURCE_HPP

#include <packetlens/packet_source.hpp>
#include <fstream>
#include <string>

namespace packetlens {
    class PcapSource : public PacketSource {
        enum class ByteOrder { LittleEndian, BigEndian };
        enum class TimestampPrecision { Microseconds, Nanoseconds };

        std::string file_name_;
        std::ifstream file_;

        ByteOrder byte_order_{ByteOrder::LittleEndian};
        TimestampPrecision timestamp_precision_{TimestampPrecision::Microseconds};
        std::uint32_t snaplen_{0};
    public:
        explicit PcapSource(std::string_view interface);

        PcapSource(const PcapSource&) = delete;
        PcapSource& operator=(const PcapSource&) = delete;
        PcapSource(PcapSource&& other) = default;
        PcapSource& operator=(PcapSource&& other) = default;

        void open() override;
        bool receive(RawFrame& frame) override;
        void close() noexcept override;

        ~PcapSource() override;
    };
}

#endif