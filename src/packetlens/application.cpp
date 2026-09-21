#include <packetlens/application.hpp>
#include <packetlens/raw_frame.hpp>
#include <iostream>

using packetlens::Application;
using packetlens::CliOptions;

int Application::run(const CliOptions& options) {
    source_ = std::make_unique<SocketSource>(options.interface);
    decoder_ = std::make_unique<Decoder>();

    source_->open();

    RawFrame frame;

    unsigned long long count = 0;
    while (source_->receive(frame)) {

        count++;

        const auto packet = decoder_->decode(frame);

        // std::cout << "Packet #" << count
        //           << " | size=" << frame.data.size()
        //           << " | valid=" << std::boolalpha << packet.valid
        //           << '\n';

        // if (packet.ethernet) {
        //     const auto& eth = *packet.ethernet;

        //     std::cout << "  Ethernet"
        //               << " | ether_type=0x"
        //               << std::hex << eth.ether_type
        //               << std::dec
        //               << '\n';
        // }

        // if (packet.ipv4) {
        //     const auto& ip = *packet.ipv4;

        //     std::cout << "  IPv4"
        //               << " | ttl=" << static_cast<int>(ip.ttl)
        //               << " | protocol=" << static_cast<int>(ip.protocol)
        //               << " | header_length="
        //               << static_cast<int>(ip.header_length)
        //               << '\n';
        // }

        // if (packet.tcp) {
        //     const auto& tcp = *packet.tcp;

        //     std::cout << "  TCP"
        //               << " | "
        //               << tcp.source_port
        //               << " -> "
        //               << tcp.destination_port
        //               << " | seq="
        //               << tcp.sequence_number
        //               << '\n';
        // }

        // if (packet.udp) {
        //     const auto& udp = *packet.udp;

        //     std::cout << "  UDP"
        //               << " | "
        //               << udp.source_port
        //               << " -> "
        //               << udp.destination_port
        //               << " | length="
        //               << udp.length
        //               << '\n';
        // }


        if (options.packet_count != 0 && options.packet_count <= count) {
            break;
        }
    }
    source_->close();

    return 0;
}
