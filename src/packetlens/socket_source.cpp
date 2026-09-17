#include <packetlens/socket_source.hpp>

#include <system_error>

using packetlens::SocketSource;

SocketSource::SocketSource(std::string_view interface) 
    :interface_(interface), fd_(-1) {

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
}

void SocketSource::receive(std::vector<uint8_t>& packet) {

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