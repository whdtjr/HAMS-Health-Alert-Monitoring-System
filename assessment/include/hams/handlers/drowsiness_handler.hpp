#pragma once

#include "hams/alert/alert_publisher.hpp"
#include "hams/assessment/drowsiness_assessor.hpp"
#include "hams/handlers/client_handler.hpp"
#include "hams/net/client_session.hpp"

namespace hams {

class DrowsinessHandler final : public ClientHandler {
public:
    DrowsinessHandler(
        ClientSession session,
        DrowsinessAssessor& assessor,
        AlertPublisher& publisher);

    void handle() override;

private:
    ClientSession session_;
    DrowsinessAssessor& assessor_;
    AlertPublisher& publisher_;
};

}  // namespace hams
