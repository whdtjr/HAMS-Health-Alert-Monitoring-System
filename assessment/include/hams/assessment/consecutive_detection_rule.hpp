#pragma once

#include "hams/assessment/assessment_rule.hpp"

#include <cstddef>

namespace hams {

class ConsecutiveDetectionRule final : public AssessmentRule {
public:
    explicit ConsecutiveDetectionRule(std::size_t threshold);

    [[nodiscard]] bool matches(
        const AssessmentContext& context) const override;

private:
    std::size_t threshold_;
};

}  // namespace hams
