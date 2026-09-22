#ifndef SOCKET_RECEIVER_HPP
#define SOCKET_RECEIVER_HPP

#include <packetlens/capture/raw_frame.hpp>
#include <packetlens/capture/packet_source.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <unistd.h>

namespace packetlens {
    /**
     * @brief Linux AF_PACKET socket implementation for live packet capture
     * 
     * SocketSource provides raw packet capture from network interfaces using
     * Linux's AF_PACKET socket mechanism. This implementation captures all Ethernet frames on the specified
     * interface in promiscuous mode.
     * 
     * @note Linux-specific implementation, not portable to other platforms
     */
    class SocketSource : public PacketSource {
        static constexpr std::size_t BUFFER_SIZE = 65536;
        std::string interface_;
        int fd_;                                          ///< File descriptor
        std::vector<std::uint8_t> buffer_;

    public:
        explicit SocketSource(std::string_view interface);

        SocketSource(const SocketSource&) = delete;
        SocketSource& operator=(const SocketSource&) = delete;
        SocketSource(SocketSource&& other) noexcept;
        SocketSource& operator=(SocketSource&& other) noexcept;

        /**
         * @throws std::logic_error if already open
         * @throws std::system_error if socket creation, binding, or configuration fails
         */
        void open() override;

        /**
         * @brief Receive a single packet from the network interface
         * @return true for a received frame; live packet sockets do not signal EOF
         * @throws std::logic_error if not open (including after being moved from)
         * @throws std::system_error if receive operation fails
         */
        bool receive(RawFrame& frame) override;
        void close() noexcept override;

        ~SocketSource() override;
    };
}

#endif
