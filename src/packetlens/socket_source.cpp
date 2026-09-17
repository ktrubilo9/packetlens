#include <packetlens/socket_source.hpp>

#include <system_error>
#include <cstring>

using packetlens::SocketSource;

SocketSource::SocketSource(std::string_view interface)
    :interface_(interface), fd_(-1), buffer_(BUFFER_SIZE) {

}

void SocketSource::open() {
    fd_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if(fd_ == -1) {
        throw std::system_error(
            errno,
            std::system_category(),
            "socket"
        );
    }

    const unsigned int ifindex = if_nametoindex(interface_.c_str());

    if (ifindex == 0) {
        close();

        throw std::system_error(
            errno,
            std::system_category(),
            "if_nametoindex"
        );
    }

    sockaddr_ll address{};
    address.sll_family = AF_PACKET;
    address.sll_protocol = htons(ETH_P_ALL);
    address.sll_ifindex = static_cast<int>(ifindex);

    if (bind(
        fd_,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)
    ) == -1) {
        close();

        throw std::system_error(
            errno,
            std::system_category(),
            "bind"
        );
    }

    packet_mreq mr{};
    mr.mr_ifindex = static_cast<int>(ifindex);
    mr.mr_type = PACKET_MR_PROMISC;

    if (setsockopt(fd_, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
        close();
        throw std::system_error(
            errno, 
            std::system_category(), 
            "setsockopt PROMISC"
        );
    }
}

void SocketSource::receive(RawFrame& frame) {
    const ssize_t n = ::recv(
        fd_,
        buffer_.data(),
        buffer_.size(),
        0
    );

    if (n < 0) {
        throw std::system_error(
            errno,
            std::system_category(),
            "recv"
        );
    }

    frame.timestamp = std::chrono::system_clock::now();
    frame.data.assign(buffer_.begin(), buffer_.begin() + n);
}

void SocketSource::close() {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

SocketSource::~SocketSource() {
    close();
}