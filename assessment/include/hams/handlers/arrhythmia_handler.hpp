#pragma once

#include "hams/alert/alert_publisher.hpp"
#include "hams/handlers/client_handler.hpp"
#include "hams/net/client_session.hpp"

namespace hams {

class ArrhythmiaHandler final : public ClientHandler {
public:
    ArrhythmiaHandler(
        ClientSession session,
        AlertPublisher& publisher);

    void handle() override;

private:
    ClientSession session_;
    AlertPublisher& publisher_;
};

}  // namespace hams
