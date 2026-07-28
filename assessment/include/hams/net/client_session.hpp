#pragma once

#include "hams/net/socket.hpp"

#include <optional>
#include <string>

namespace hams {

class ClientSession {
public:
    explicit ClientSession(Socket socket);

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;
    ClientSession(ClientSession&&) noexcept = default;
    ClientSession& operator=(ClientSession&&) noexcept = default;

    [[nodiscard]] std::optional<std::string> receiveMessage();

private:
    [[nodiscard]] std::optional<std::string> extractMessage();

    Socket socket_;
    std::string pending_;
};

}  // namespace hams
