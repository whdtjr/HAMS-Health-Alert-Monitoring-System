#include "hams/server/assessment_server.hpp"

#include "hams/net/client_session.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

namespace hams {

AssessmentServer::AssessmentServer(
    std::uint16_t port,
    const MessageParser& parser,
    const ClientHandlerFactory& handlerFactory)
    : listener_{port},
      parser_{parser},
      handlerFactory_{handlerFactory} {}

void AssessmentServer::removeCompletedWorkers() {
    std::erase_if(workers_, [](std::future<void>& worker) {
        if (worker.wait_for(std::chrono::seconds{0}) !=
            std::future_status::ready) {
            return false;
        }
        worker.get();
        return true;
    });
}

void AssessmentServer::run() {
    while (true) {
        try {
            removeCompletedWorkers();
            ClientSession session{listener_.accept()};
            const auto handshake = session.receiveMessage();
            if (!handshake) {
                continue;
            }

            const ClientType type = parser_.parseClientType(*handshake);
            auto handler =
                handlerFactory_.create(type, std::move(session));

            workers_.push_back(std::async(
                std::launch::async,
                [handler = std::move(handler)]() mutable {
                    try {
                        handler->handle();
                    } catch (const std::exception& error) {
                        std::cerr << "client handler failed: "
                                  << error.what() << '\n';
                    }
                }));
        } catch (const std::exception& error) {
            std::cerr << "connection rejected: "
                      << error.what() << '\n';
        }
    }
}

}  // namespace hams
