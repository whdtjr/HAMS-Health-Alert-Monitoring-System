#include "hams/handlers/arrhythmia_handler.hpp"

#include <utility>

namespace hams {

ArrhythmiaHandler::ArrhythmiaHandler(
    ClientSession session,
    AlertPublisher& publisher)
    : session_{std::move(session)},
      publisher_{publisher} {}

void ArrhythmiaHandler::handle() {
    publisher_.publish(AlertType::emergency);
}

}  // namespace hams
