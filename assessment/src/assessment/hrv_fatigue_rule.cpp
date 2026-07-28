#include "hams/assessment/hrv_fatigue_rule.hpp"

#include <stdexcept>

namespace hams {

HrvFatigueRule::HrvFatigueRule() = default;

HrvFatigueRule::HrvFatigueRule(Thresholds thresholds)
    : thresholds_{thresholds} {}

HrvMetrics HrvFatigueRule::averageOf(
    const std::vector<PpgData>& samples)
{
    if (samples.empty()) {
        throw std::invalid_argument{"cannot average empty HRV samples"};
    }

    HrvMetrics total{};
    for (const auto& sample : samples) {
        total.sdnn += sample.sdnn;
        total.rmssd += sample.rmssd;
        total.pnn50 += sample.pnn50;
    }

    const auto count = static_cast<double>(samples.size());
    return {
        total.sdnn / count,
        total.rmssd / count,
        total.pnn50 / count
    };
}

bool HrvFatigueRule::matches(
    const AssessmentContext& context) const
{
    if (context.ppgSamples.empty()) {
        return false;
    }

    const auto average = averageOf(context.ppgSamples);
    return average.sdnn < thresholds_.sdnn ||
           average.rmssd < thresholds_.rmssd ||
           average.pnn50 < thresholds_.pnn50;
}

}  // namespace hams
