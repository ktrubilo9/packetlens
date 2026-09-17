#include <packetlens/application.hpp>

using packetlens::Application;
using packetlens::CliOptions;

int Application::run(const CliOptions& options) {
    source_ = std::make_unique<SocketSource>(options.interface);

    source_->open();
    source_->close();

    return 0;
}