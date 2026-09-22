# PacketLens

A command-line packet sniffer for Linux, written in C++17. Captures traffic from
a network interface using raw sockets and prints Ethernet, IPv4, TCP, and UDP
header information.

## Features

- Live packet capture from a selected network interface.
- Classic PCAP 2.4 input with Ethernet frames.
- Ethernet II and IPv4 header decoding.
- TCP ports, sequence numbers, and UDP datagram lengths.
- Optional packet count limit.

## Requirements

- Linux
- C++17 compiler
- CMake 3.19 or newer
- Git

Live capture requires root privileges or `CAP_NET_RAW`.

## Build

```sh
git clone --recurse-submodules https://github.com/ktrubilo9/packetlens.git
cd packetlens
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

For an existing clone, fetch fmt and Catch2 with `git submodule update --init --recursive`.
To build without tests, add `-DBUILD_TESTING=OFF` to the CMake configuration command.

## Usage

Capture 10 packets from an interface:

```sh
sudo ./build/packetlens --interface eth0 --count 10
```

Replace `eth0` with your interface name. Omit `--count` to capture until interrupted
with Ctrl+C.

Read a capture file (no root privileges required):

```sh
./build/packetlens --read tests/fixtures/sample_udp.pcap
```

Packet timestamps are displayed in UTC with microsecond precision.

| Option | Description |
| --- | --- |
| `-i, --interface <name>` | Network interface to capture from |
| `-r, --read <file>` | Read Ethernet packets from a classic PCAP file |
| `-c, --count <number>` | Stop after N packets; 0 means unlimited |
| `-h, --help` | Show help |
| `-V, --version` | Show version |

## Tests

Tests use Catch2 and synthetic packet data. No network access or root privileges
are required to run them.

```sh
ctest --test-dir build --output-on-failure
```

Run parser or decoder tests separately:

```sh
./build/tests/packetlens_tests '[cli]'
./build/tests/packetlens_tests '[decoder]'
```

GitHub Actions builds and runs the tests on Ubuntu with AddressSanitizer and
UndefinedBehaviorSanitizer.

## Limitations

- BPF filters, file/JSON output, and quiet/verbose modes are not
  implemented, although their options appear in `--help`.
- IPv6, ARP, and ICMP are identified but their headers are not decoded.
- Checksum validation and IP fragment reassembly are not supported.
- PCAPNG is not supported; captured PCAP frames are limited to 65,536 bytes.
