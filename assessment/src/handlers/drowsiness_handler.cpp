#include "hams/handlers/drowsiness_handler.hpp"

#include <utility>

namespace hams {

DrowsinessHandler::DrowsinessHandler(
    ClientSession session,
    DrowsinessAssessor& assessor,
    AlertPublisher& publisher)
    : session_{std::move(session)},
      assessor_{assessor},
      publisher_{publisher} {}

void DrowsinessHandler::handle() {
    if (assessor_.onCameraDetection()) {
        publisher_.publish(AlertType::drowsiness);
    }
}

}  // namespace hams
