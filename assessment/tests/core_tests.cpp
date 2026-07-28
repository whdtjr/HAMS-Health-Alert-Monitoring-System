#include "hams/assessment/consecutive_detection_rule.hpp"
#include "hams/assessment/drowsiness_assessor.hpp"
#include "hams/assessment/hrv_fatigue_rule.hpp"
#include "hams/concurrency/blocking_queue.hpp"
#include "hams/domain/client_type.hpp"
#include "hams/protocol/json_message_parser.hpp"
#include "hams/repository/ppg_repository.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error{std::string{message}};
    }
}

hams::PpgData sample(double sdnn, double rmssd, double pnn50) {
    return {
        std::chrono::system_clock::now(),
        512,
        70,
        850,
        sdnn,
        rmssd,
        pnn50
    };
}

void blockingQueueOverwritesOldestValue() {
    hams::BlockingQueue<int> queue{3};
    queue.pushOverwrite(1);
    queue.pushOverwrite(2);
    queue.pushOverwrite(3);
    queue.pushOverwrite(4);

    const auto values = queue.snapshot();
    require(values.size() == 3, "queue must preserve its capacity");
    require(values[0] == 2 && values[1] == 3 && values[2] == 4,
            "queue must remove the oldest value");
}

void hrvRuleUsesAverageThresholds() {
    hams::HrvFatigueRule rule;

    const hams::AssessmentContext normal{
        {sample(45.0, 32.0, 0.35), sample(35.0, 28.0, 0.30)},
        1
    };
    require(!rule.matches(normal), "normal HRV must not indicate fatigue");

    const hams::AssessmentContext fatigued{
        {sample(20.0, 32.0, 0.35), sample(30.0, 28.0, 0.30)},
        1
    };
    require(rule.matches(fatigued), "low average SDNN must indicate fatigue");
}

void drowsinessAssessorComposesRulesAndResetsCount() {
    hams::PpgRepository repository{30};
    hams::DrowsinessAssessor assessor{repository};
    assessor.addRule(std::make_unique<hams::HrvFatigueRule>());
    assessor.addRule(
        std::make_unique<hams::ConsecutiveDetectionRule>(3));

    require(!assessor.onCameraDetection(), "first detection must not alert");
    require(!assessor.onCameraDetection(), "second detection must not alert");
    require(assessor.onCameraDetection(), "third detection must alert");
    require(!assessor.onCameraDetection(), "counter must reset after alert");

    repository.save(sample(10.0, 10.0, 0.10));
    require(assessor.onCameraDetection(), "fatigued HRV must alert immediately");
}

void jsonParserValidatesAndConvertsMessages() {
    hams::JsonMessageParser parser;

    require(
        parser.parseClientType(R"({"id":"drowsy"})") ==
            hams::ClientType::drowsiness,
        "drowsy ID must map to drowsiness client");

    const auto data = parser.parsePpgData(
        R"({"signal":510,"bpm":72,"ibi":833,"sdnn":42.5,"rmssd":31.2,"pnn50":0.33,"timestamp":1720000000})");

    require(data.signal == 510, "signal must be parsed");
    require(data.bpm == 72, "BPM must be parsed");
    require(std::abs(data.sdnn - 42.5) < 0.0001, "SDNN must be parsed");

    bool rejected = false;
    try {
        static_cast<void>(
            parser.parsePpgData(R"({"signal":510})"));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "missing fields must be rejected");
}

}  // namespace

int main() {
    try {
        blockingQueueOverwritesOldestValue();
        hrvRuleUsesAverageThresholds();
        drowsinessAssessorComposesRulesAndResetsCount();
        jsonParserValidatesAndConvertsMessages();
        std::cout << "All HAMS core tests passed.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
