#include <packetlens/socket_source.hpp>

#include <system_error>
#include <cerrno>
#include <stdexcept>
#include <utility>

using packetlens::SocketSource;

SocketSource::SocketSource(std::string_view interface)
    :interface_(interface), fd_(-1), buffer_(BUFFER_SIZE) {

}

SocketSource::SocketSource(SocketSource&& other) noexcept
    : interface_(std::move(other.interface_)),
      fd_(std::exchange(other.fd_, -1)),
      buffer_(std::move(other.buffer_)) {
}

SocketSource& SocketSource::operator=(SocketSource&& other) noexcept {
    if (this != &other) {
        close();
        interface_ = std::move(other.interface_);
        buffer_ = std::move(other.buffer_);
        fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
}

void SocketSource::open() {
    if (fd_ != -1) {
        throw std::logic_error("socket source is already open");
    }
    // A moved-from vector may be empty. Restore storage before acquiring a socket.
    buffer_.resize(BUFFER_SIZE);
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
        const int error = errno;
        close();

        throw std::system_error(
            error,
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
        const int error = errno;
        close();

        throw std::system_error(
            error,
            std::system_category(),
            "bind"
        );
    }

    packet_mreq mr{};
    mr.mr_ifindex = static_cast<int>(ifindex);
    mr.mr_type = PACKET_MR_PROMISC;

    if (setsockopt(fd_, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
        const int error = errno;
        close();
        throw std::system_error(
            error,
            std::system_category(), 
            "setsockopt PROMISC"
        );
    }
}

bool SocketSource::receive(RawFrame& frame) {
    if (fd_ == -1) {
        throw std::logic_error("socket source is not open");
    }

    ssize_t n;
    do {
        n = ::recv(fd_, buffer_.data(), buffer_.size(), 0);
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        throw std::system_error(
            errno,
            std::system_category(),
            "recv"
        );
    }

    frame.data.assign(buffer_.begin(), buffer_.begin() + n);
    frame.timestamp = std::chrono::system_clock::now();
    return true;
}

void SocketSource::close() noexcept {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

SocketSource::~SocketSource() {
    close();
}
