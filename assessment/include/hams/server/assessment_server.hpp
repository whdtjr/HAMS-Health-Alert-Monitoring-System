#pragma once

#include "hams/handlers/client_handler_factory.hpp"
#include "hams/net/tcp_listener.hpp"
#include "hams/protocol/message_parser.hpp"

#include <cstdint>
#include <future>
#include <vector>

namespace hams {

class AssessmentServer {
public:
    AssessmentServer(
        std::uint16_t port,
        const MessageParser& parser,
        const ClientHandlerFactory& handlerFactory);

    void run();

private:
    void removeCompletedWorkers();

    TcpListener listener_;
    const MessageParser& parser_;
    const ClientHandlerFactory& handlerFactory_;
    std::vector<std::future<void>> workers_;
};

}  // namespace hams
