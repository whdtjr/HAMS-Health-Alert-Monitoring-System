#pragma once

#include "hams/assessment/assessment_rule.hpp"

namespace hams {

class HrvFatigueRule final : public AssessmentRule {
public:
    struct Thresholds {
        double sdnn{30.0};
        double rmssd{20.0};
        double pnn50{0.25};
    };

    HrvFatigueRule();
    explicit HrvFatigueRule(Thresholds thresholds);

    [[nodiscard]] bool matches(
        const AssessmentContext& context) const override;

    [[nodiscard]] static HrvMetrics averageOf(
        const std::vector<PpgData>& samples);

private:
    Thresholds thresholds_;
};

}  // namespace hams
