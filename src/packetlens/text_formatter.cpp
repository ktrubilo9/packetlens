#include <packetlens/text_formatter.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <array>
#include <chrono>
#include <cstdint>

using packetlens::TextFormatter;

namespace {
    std::string format_ipv4(const std::array<std::uint8_t, 4>& address) {
        return fmt::format("{}.{}.{}.{}",
            address[0], address[1], address[2], address[3]);
    }

    std::string format_timestamp(
        std::chrono::system_clock::time_point timestamp
    ) {
        using namespace std::chrono;

        const auto whole_seconds = floor<seconds>(timestamp);
        const auto fractional_us = duration_cast<microseconds>(timestamp - whole_seconds).count();
        const auto time = system_clock::to_time_t(whole_seconds);
        return fmt::format("{:%H:%M:%S}.{:06}",
            fmt::gmtime(time), fractional_us);
    }
}

std::string TextFormatter::format(const RawFrame& frame, const DecodedPacket& packet) {
    const auto time = format_timestamp(frame.timestamp);

    if (!packet.ethernet) {
        return fmt::format(
            "{} [incomplete Ethernet header], captured {} bytes",
            time, frame.data.size()
        );
    }

    if (packet.ipv4) {
        const auto& ip = *packet.ipv4;
        const auto source = format_ipv4(ip.source);
        const auto destination = format_ipv4(ip.destination);

        if (packet.tcp) {
            const auto& tcp = *packet.tcp;

            return fmt::format(
                "{} IP {}.{} > {}.{}: TCP, seq {}, ack {}",
                time,
                source, tcp.source_port,
                destination, tcp.destination_port,
                tcp.sequence_number,
                tcp.acknowledgment_number
            );
        }

        if (packet.udp) {
            const auto& udp = *packet.udp;

            return fmt::format(
                "{} IP {}.{} > {}.{}: UDP, length {}",
                time,
                source, udp.source_port,
                destination, udp.destination_port,
                udp.length - 8
            );
        }

        return fmt::format(
            "{} IP {} > {}: proto {}, length {}",
            time, source, destination,
            ip.protocol, ip.total_length
        );
    }

    return fmt::format(
        "{} Ethernet, ethertype 0x{:04x}, captured {} bytes",
        time,
        packet.ethernet->ether_type,
        frame.data.size()
    );
}
