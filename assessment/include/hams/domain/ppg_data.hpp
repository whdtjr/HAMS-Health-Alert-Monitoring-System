#pragma once

#include <chrono>

namespace hams {

struct PpgData {
    std::chrono::system_clock::time_point timestamp{};
    int signal{};
    int bpm{};
    int ibi{};
    double sdnn{};
    double rmssd{};
    double pnn50{};
};

struct HrvMetrics {
    double sdnn{};
    double rmssd{};
    double pnn50{};
};

}  // namespace hams
