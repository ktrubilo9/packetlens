#include <packetlens/application.hpp>
#include <packetlens/raw_frame.hpp>
#include <packetlens/text_formatter.hpp>
#include <iostream>

using packetlens::Application;
using packetlens::CliOptions;

int Application::run(const CliOptions& options) {
    if (!options.inputFile.empty()) {
        source_ = std::make_unique<PcapSource>(options.inputFile);
    } else {
        source_ = std::make_unique<SocketSource>(options.interface);
    }
    decoder_ = std::make_unique<Decoder>();

    source_->open();

    RawFrame frame;

    unsigned long long count = 0;
    while (source_->receive(frame)) {
        count++;
        const auto packet = decoder_->decode(frame);

        std::cout << TextFormatter::format(frame, packet) << "\n";

        if (options.packet_count != 0 && options.packet_count <= count) {
            break;
        }
    }
    source_->close();

    return 0;
}
