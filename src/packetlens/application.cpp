#include <packetlens/application.hpp>
#include <packetlens/raw_frame.hpp>
#include <iostream>

using packetlens::Application;
using packetlens::CliOptions;

int Application::run(const CliOptions& options) {
    source_ = std::make_unique<SocketSource>(options.interface);

    source_->open();

    RawFrame frame;
    unsigned long long count = 0;
    while(true) {
        source_->receive(frame);

        //std::cout << "got frame, size=" << frame.data.size() << "\n";

        count++;

        if (options.packet_count != 0 && options.packet_count >= count) {
            break;
        }
    }
    source_->close();

    return 0;
}