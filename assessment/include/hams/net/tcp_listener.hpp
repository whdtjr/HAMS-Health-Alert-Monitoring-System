#pragma once

#include "hams/net/socket.hpp"

#include <cstdint>

namespace hams {

class TcpListener {
public:
    explicit TcpListener(std::uint16_t port, int backlog = 16);
    [[nodiscard]] Socket accept() const;

private:
    Socket socket_;
};

}  // namespace hams
