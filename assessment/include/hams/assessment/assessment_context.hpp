#pragma once

#include "hams/domain/ppg_data.hpp"

#include <cstddef>
#include <vector>

namespace hams {

struct AssessmentContext {
    std::vector<PpgData> ppgSamples;
    std::size_t consecutiveCameraDetections{};
};

}  // namespace hams
