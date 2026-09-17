#ifndef SOCKET_RECEIVER_HPP
#define SOCKET_RECEIVER_HPP

#include <packetlens/packet_source.hpp>

#include <string>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <unistd.h>

namespace packetlens {
    class SocketSource : public PacketSource {
        std::string interface_;
        int fd_; ///file descriptor
    public:
        explicit SocketSource(std::string_view interface);

        void open() override;
        void receive(std::vector<uint8_t>& packet) override;
        void close() override;

        ~SocketSource();
    };
}

#endif