#include "hams/assessment/consecutive_detection_rule.hpp"

#include <stdexcept>

namespace hams {

ConsecutiveDetectionRule::ConsecutiveDetectionRule(std::size_t threshold)
    : threshold_{threshold}
{
    if (threshold == 0) {
        throw std::invalid_argument{"detection threshold must be positive"};
    }
}

bool ConsecutiveDetectionRule::matches(
    const AssessmentContext& context) const
{
    return context.consecutiveCameraDetections >= threshold_;
}

}  // namespace hams
