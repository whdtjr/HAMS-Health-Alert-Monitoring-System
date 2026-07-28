#pragma once

#include "hams/domain/client_type.hpp"
#include "hams/handlers/client_handler.hpp"
#include "hams/net/client_session.hpp"

#include <functional>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace hams {

class ClientHandlerFactory {
public:
    using Creator =
        std::function<std::unique_ptr<ClientHandler>(ClientSession)>;

    void registerHandler(ClientType type, Creator creator) {
        if (!creator) {
            throw std::invalid_argument{"handler creator cannot be empty"};
        }

        const auto [unused, inserted] =
            creators_.emplace(type, std::move(creator));
        if (!inserted) {
            throw std::logic_error{"handler type is already registered"};
        }
    }

    [[nodiscard]] std::unique_ptr<ClientHandler> create(
        ClientType type,
        ClientSession session) const
    {
        const auto creator = creators_.find(type);
        if (creator == creators_.end()) {
            throw std::invalid_argument{"handler type is not registered"};
        }
        return creator->second(std::move(session));
    }

private:
    std::unordered_map<ClientType, Creator> creators_;
};

}  // namespace hams
