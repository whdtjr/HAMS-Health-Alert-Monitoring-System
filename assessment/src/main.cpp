#include "hams/alert/fifo_alert_publisher.hpp"
#include "hams/assessment/consecutive_detection_rule.hpp"
#include "hams/assessment/drowsiness_assessor.hpp"
#include "hams/assessment/hrv_fatigue_rule.hpp"
#include "hams/handlers/arrhythmia_handler.hpp"
#include "hams/handlers/client_handler_factory.hpp"
#include "hams/handlers/drowsiness_handler.hpp"
#include "hams/handlers/hrv_handler.hpp"
#include "hams/protocol/json_message_parser.hpp"
#include "hams/repository/ppg_repository.hpp"
#include "hams/server/assessment_server.hpp"

#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {

std::uint16_t parsePort(std::string_view value) {
    unsigned int port{};
    const auto result =
        std::from_chars(value.data(), value.data() + value.size(), port);

    if (result.ec != std::errc{} ||
        result.ptr != value.data() + value.size() ||
        port == 0 || port > 65535) {
        throw std::invalid_argument{"port must be between 1 and 65535"};
    }
    return static_cast<std::uint16_t>(port);
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    try {
        hams::PpgRepository ppgRepository{30};

        hams::DrowsinessAssessor drowsinessAssessor{ppgRepository};
        drowsinessAssessor.addRule(
            std::make_unique<hams::HrvFatigueRule>());
        drowsinessAssessor.addRule(
            std::make_unique<hams::ConsecutiveDetectionRule>(3));

        hams::JsonMessageParser parser;
        hams::FifoAlertPublisher alertPublisher{"/tmp/symptom_pipe"};
        hams::ClientHandlerFactory factory;

        factory.registerHandler(
            hams::ClientType::hrv,
            [&](hams::ClientSession session) {
                return std::make_unique<hams::HrvHandler>(
                    std::move(session),
                    ppgRepository,
                    parser);
            });

        factory.registerHandler(
            hams::ClientType::drowsiness,
            [&](hams::ClientSession session) {
                return std::make_unique<hams::DrowsinessHandler>(
                    std::move(session),
                    drowsinessAssessor,
                    alertPublisher);
            });

        factory.registerHandler(
            hams::ClientType::arrhythmia,
            [&](hams::ClientSession session) {
                return std::make_unique<hams::ArrhythmiaHandler>(
                    std::move(session),
                    alertPublisher);
            });

        hams::AssessmentServer server{
            parsePort(argv[1]),
            parser,
            factory};
        server.run();
    } catch (const std::exception& error) {
        std::cerr << "fatal error: " << error.what() << '\n';
        return 1;
    }
}
