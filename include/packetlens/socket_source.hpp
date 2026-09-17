#ifndef SOCKET_RECEIVER_HPP
#define SOCKET_RECEIVER_HPP

#include <packetlens/raw_frame.hpp>
#include <packetlens/packet_source.hpp>

#include <string>
#include <vector>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <unistd.h>

namespace packetlens {
    class SocketSource : public PacketSource {
        static constexpr std::size_t BUFFER_SIZE = 65536;
        std::string interface_;
        int fd_; ///file descriptor

        std::vector<std::uint8_t> buffer_;
    public:
        explicit SocketSource(const std::string& interface);

        void open() override;
        void receive(RawFrame& frame) override;
        void close() override;

        ~SocketSource();
    };
}

#endif