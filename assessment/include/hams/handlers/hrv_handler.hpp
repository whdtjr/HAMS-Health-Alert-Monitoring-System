#pragma once

#include "hams/handlers/client_handler.hpp"
#include "hams/net/client_session.hpp"
#include "hams/protocol/message_parser.hpp"
#include "hams/repository/ppg_repository.hpp"

namespace hams {

class HrvHandler final : public ClientHandler {
public:
    HrvHandler(
        ClientSession session,
        PpgRepository& repository,
        const MessageParser& parser);

    void handle() override;

private:
    ClientSession session_;
    PpgRepository& repository_;
    const MessageParser& parser_;
};

}  // namespace hams
