#include <packetlens/capture/socket_source.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <utility>

using packetlens::RawFrame;
using packetlens::SocketSource;

static_assert(!std::is_copy_constructible_v<SocketSource>);
static_assert(!std::is_copy_assignable_v<SocketSource>);
static_assert(std::is_nothrow_move_constructible_v<SocketSource>);
static_assert(std::is_nothrow_move_assignable_v<SocketSource>);

namespace {
enum class Failure { none, socket, interface, bind, membership, receive };
struct FakeSocket {
    Failure failure = Failure::none;
    int next_fd = 10000;
    int socket_calls = 0;
    int receive_calls = 0;
    int received_fd = -1;
    std::size_t receive_capacity = 0;
    bool interrupt_once = false;
    std::vector<int> closed;
    std::vector<std::uint8_t> payload{0x01, 0x80, 0xff};
    std::string interface;
} fake;

struct SocketFixture {
    SocketFixture() { fake = FakeSocket{}; }
};

int fail() { errno = EACCES; return -1; }
}

// GNU linker wrappers exercise the production class with deterministic syscall results.
extern "C" int __wrap_socket(int, int, int) {
    ++fake.socket_calls;
    return fake.failure == Failure::socket ? fail() : fake.next_fd++;
}
extern "C" unsigned int __wrap_if_nametoindex(const char* name) {
    fake.interface = name;
    if (fake.failure == Failure::interface) { fail(); return 0; }
    return 1;
}
extern "C" int __wrap_bind(int, const sockaddr*, socklen_t) {
    return fake.failure == Failure::bind ? fail() : 0;
}
extern "C" int __wrap_setsockopt(int, int, int, const void*, socklen_t) {
    return fake.failure == Failure::membership ? fail() : 0;
}
extern "C" ssize_t __wrap_recv(int fd, void* buffer, std::size_t size, int) {
    ++fake.receive_calls;
    fake.received_fd = fd;
    fake.receive_capacity = size;
    if (std::exchange(fake.interrupt_once, false)) { errno = EINTR; return -1; }
    if (fake.failure == Failure::receive) { errno = EIO; return -1; }
    const auto count = std::min(size, fake.payload.size());
    if (count != 0) { std::memcpy(buffer, fake.payload.data(), count); }
    return static_cast<ssize_t>(count);
}
extern "C" int __real_close(int);
extern "C" int __wrap_close(int fd) {
    if (fd < 10000 || fd >= fake.next_fd) { return __real_close(fd); }
    fake.closed.push_back(fd);
    errno = EBADF; // Cleanup must not overwrite the original setup error.
    return 0;
}

TEST_CASE_METHOD(SocketFixture, "SocketSource closes once and can reopen", "[socket]") {
    {
        SocketSource source("eth0");
        source.close();
        CHECK(fake.closed.empty());
        source.open();
        CHECK(fake.interface == "eth0");
        source.close();
        source.close();
        CHECK(fake.closed == std::vector<int>{10000});
        source.open();
    }
    CHECK(fake.closed == (std::vector<int>{10000, 10001}));
}

TEST_CASE_METHOD(SocketFixture, "SocketSource rejects double open without losing its socket", "[socket]") {
    SocketSource source("eth0");
    source.open();
    CHECK_THROWS_AS(source.open(), std::logic_error);
    CHECK(fake.socket_calls == 1);
    CHECK(fake.closed.empty());
    RawFrame frame;
    REQUIRE(source.receive(frame));
    CHECK(fake.received_fd == 10000);
}

TEST_CASE_METHOD(SocketFixture, "SocketSource move construction transfers ownership and buffer", "[socket]") {
    {
        SocketSource original("eth0");
        original.open();
        {
            SocketSource moved(std::move(original));
            original.close();
            CHECK(fake.closed.empty());
            RawFrame frame;
            CHECK_THROWS_AS(original.receive(frame), std::logic_error);
            REQUIRE(moved.receive(frame));
            CHECK(fake.received_fd == 10000);
            CHECK(fake.receive_capacity == 65536);
            CHECK(frame.data == fake.payload);
        }
        CHECK(fake.closed == std::vector<int>{10000});
    }
    CHECK(fake.closed == std::vector<int>{10000});
}

TEST_CASE_METHOD(SocketFixture, "SocketSource move assignment releases the previous socket", "[socket]") {
    {
        SocketSource source("eth0"), destination("lo");
        source.open();
        destination.open();
        destination = std::move(source);
        CHECK(fake.closed == std::vector<int>{10001});
        RawFrame frame;
        CHECK_THROWS_AS(source.receive(frame), std::logic_error);
        REQUIRE(destination.receive(frame));
        CHECK(fake.received_fd == 10000);
        CHECK(frame.data == fake.payload);
        destination.close();
        destination.open();
        CHECK(fake.interface == "eth0");
    }
    CHECK(fake.closed == (std::vector<int>{10001, 10000, 10002}));
}

TEST_CASE_METHOD(SocketFixture, "SocketSource self move preserves its socket", "[socket]") {
    SocketSource source("eth0");
    source.open();
    auto& alias = source;
    source = std::move(alias);
    CHECK(fake.closed.empty());
    RawFrame frame;
    REQUIRE(source.receive(frame));
    CHECK(fake.received_fd == 10000);
}

TEST_CASE_METHOD(SocketFixture, "Closed and moved-from SocketSources can be assigned new values", "[socket]") {
    SocketSource source("eth0");
    SocketSource destination(std::move(source));
    destination.open();
    source = SocketSource("lo");
    source.open();
    CHECK(fake.interface == "lo");
    destination = SocketSource("eth1");
    CHECK(fake.closed == std::vector<int>{10000});
    destination.open();
    CHECK(fake.interface == "eth1");
    RawFrame frame;
    REQUIRE(destination.receive(frame));
    CHECK(fake.receive_capacity == 65536);
}

TEST_CASE_METHOD(SocketFixture, "SocketSource setup errors release resources and preserve errno", "[socket]") {
    for (const auto failure : {Failure::socket, Failure::interface, Failure::bind, Failure::membership}) {
        fake = FakeSocket{};
        fake.failure = failure;
        SocketSource source("eth0");
        try {
            source.open();
            FAIL("open should throw");
        } catch (const std::system_error& error) {
            CHECK(error.code().value() == EACCES);
        }
        CHECK(fake.closed.size() == (failure == Failure::socket ? 0u : 1u));
        fake.failure = Failure::none;
        REQUIRE_NOTHROW(source.open());
    }
}

TEST_CASE_METHOD(SocketFixture, "SocketSource receive requires an open source", "[socket]") {
    SocketSource source("eth0");
    RawFrame frame;
    CHECK_THROWS_AS(source.receive(frame), std::logic_error);
    source.open();
    source.close();
    CHECK_THROWS_AS(source.receive(frame), std::logic_error);
    CHECK(fake.receive_calls == 0);
}

TEST_CASE_METHOD(SocketFixture, "SocketSource receive retries EINTR and replaces frame contents", "[socket]") {
    SocketSource source("eth0");
    source.open();
    fake.interrupt_once = true;
    RawFrame frame;
    frame.data.assign(20, 0);
    REQUIRE(source.receive(frame));
    CHECK(fake.receive_calls == 2);
    CHECK(frame.data == fake.payload);
    CHECK(frame.timestamp != std::chrono::system_clock::time_point{});
}

TEST_CASE_METHOD(SocketFixture, "SocketSource receive errors preserve the previous frame", "[socket]") {
    SocketSource source("eth0");
    source.open();
    fake.failure = Failure::receive;
    RawFrame frame;
    frame.data = {1, 2};
    frame.timestamp = std::chrono::system_clock::time_point{};
    CHECK_THROWS_AS(source.receive(frame), std::system_error);
    CHECK(frame.data == (std::vector<std::uint8_t>{1, 2}));
    CHECK(frame.timestamp == std::chrono::system_clock::time_point{});
    CHECK(fake.closed.empty());
}

TEST_CASE_METHOD(SocketFixture, "Empty packet datagrams are frames rather than EOF", "[socket]") {
    SocketSource source("eth0");
    source.open();
    fake.payload.clear();
    RawFrame frame;
    frame.data = {1, 2};
    REQUIRE(source.receive(frame));
    CHECK(frame.data.empty());
}
