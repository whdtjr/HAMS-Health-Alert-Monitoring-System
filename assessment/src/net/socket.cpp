#include "hams/net/socket.hpp"

#include <utility>

#include <unistd.h>

namespace hams {

Socket::Socket(int descriptor) noexcept : descriptor_{descriptor} {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept
    : descriptor_{std::exchange(other.descriptor_, -1)} {}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        descriptor_ = std::exchange(other.descriptor_, -1);
    }
    return *this;
}

int Socket::nativeHandle() const noexcept {
    return descriptor_;
}

bool Socket::valid() const noexcept {
    return descriptor_ >= 0;
}

void Socket::close() noexcept {
    if (valid()) {
        ::close(descriptor_);
        descriptor_ = -1;
    }
}

}  // namespace hams
