#include "hams/net/tcp_listener.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include <arpa/inet.h>
#include <sys/socket.h>

namespace hams {
namespace {

Socket createListener(std::uint16_t port, int backlog) {
    Socket socket{::socket(AF_INET, SOCK_STREAM, 0)};
    if (!socket.valid()) {
        throw std::runtime_error{"failed to create server socket"};
    }

    int reuse = 1;
    ::setsockopt(
        socket.nativeHandle(),
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (::bind(
            socket.nativeHandle(),
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1) {
        throw std::runtime_error{
            "failed to bind server socket: " + std::string{std::strerror(errno)}};
    }

    if (::listen(socket.nativeHandle(), backlog) == -1) {
        throw std::runtime_error{"failed to listen on server socket"};
    }
    return socket;
}

}  // namespace

TcpListener::TcpListener(std::uint16_t port, int backlog)
    : socket_{createListener(port, backlog)} {}

Socket TcpListener::accept() const {
    const int client = ::accept(socket_.nativeHandle(), nullptr, nullptr);
    if (client == -1) {
        throw std::runtime_error{"failed to accept client"};
    }
    return Socket{client};
}

}  // namespace hams
