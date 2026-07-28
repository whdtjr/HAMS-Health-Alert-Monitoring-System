#include "hams/alert/fifo_alert_publisher.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string_view>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace hams {
namespace {

std::string_view messageFor(AlertType alert) {
    switch (alert) {
    case AlertType::drowsiness:
        return "drowsiness\n";
    case AlertType::emergency:
        return "emergency\n";
    }
    throw std::invalid_argument{"unsupported alert type"};
}

class FileDescriptor {
public:
    explicit FileDescriptor(int value) : value_{value} {}
    ~FileDescriptor() {
        if (value_ >= 0) {
            ::close(value_);
        }
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    [[nodiscard]] int get() const noexcept {
        return value_;
    }

private:
    int value_;
};

}  // namespace

FifoAlertPublisher::FifoAlertPublisher(std::filesystem::path path)
    : path_{std::move(path)} {}

void FifoAlertPublisher::publish(AlertType alert) {
    if (::mkfifo(path_.c_str(), 0666) == -1 && errno != EEXIST) {
        throw std::runtime_error{
            "failed to create alert FIFO: " + std::string{std::strerror(errno)}};
    }

    FileDescriptor descriptor{
        ::open(path_.c_str(), O_WRONLY | O_NONBLOCK)};
    if (descriptor.get() == -1) {
        throw std::runtime_error{
            "failed to open alert FIFO: " + std::string{std::strerror(errno)}};
    }

    const auto message = messageFor(alert);
    const auto written = ::write(
        descriptor.get(),
        message.data(),
        message.size());

    if (written != static_cast<ssize_t>(message.size())) {
        throw std::runtime_error{"failed to write complete alert message"};
    }
}

}  // namespace hams
