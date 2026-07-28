#include "hams/assessment/drowsiness_assessor.hpp"

#include <algorithm>
#include <stdexcept>

namespace hams {

DrowsinessAssessor::DrowsinessAssessor(PpgRepository& repository)
    : repository_{repository} {}

void DrowsinessAssessor::addRule(std::unique_ptr<AssessmentRule> rule) {
    if (!rule) {
        throw std::invalid_argument{"assessment rule cannot be null"};
    }
    rules_.push_back(std::move(rule));
}

bool DrowsinessAssessor::onCameraDetection() {
    std::lock_guard lock{mutex_};
    ++consecutiveDetections_;

    const AssessmentContext context{
        repository_.snapshot(),
        consecutiveDetections_
    };

    const bool detected = std::ranges::any_of(
        rules_,
        [&context](const auto& rule) {
            return rule->matches(context);
        });

    if (detected) {
        consecutiveDetections_ = 0;
    }
    return detected;
}

}  // namespace hams
