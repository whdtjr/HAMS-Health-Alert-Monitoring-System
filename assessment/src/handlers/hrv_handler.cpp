#include "hams/handlers/hrv_handler.hpp"

#include <utility>

namespace hams {

HrvHandler::HrvHandler(
    ClientSession session,
    PpgRepository& repository,
    const MessageParser& parser)
    : session_{std::move(session)},
      repository_{repository},
      parser_{parser} {}

void HrvHandler::handle() {
    while (auto message = session_.receiveMessage()) {
        repository_.save(parser_.parsePpgData(*message));
    }
}

}  // namespace hams
