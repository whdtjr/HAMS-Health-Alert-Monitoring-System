#pragma once

#include "hams/assessment/assessment_rule.hpp"
#include "hams/repository/ppg_repository.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace hams {

class DrowsinessAssessor {
public:
    explicit DrowsinessAssessor(PpgRepository& repository);

    void addRule(std::unique_ptr<AssessmentRule> rule);
    [[nodiscard]] bool onCameraDetection();

private:
    PpgRepository& repository_;
    std::vector<std::unique_ptr<AssessmentRule>> rules_;
    std::mutex mutex_;
    std::size_t consecutiveDetections_{0};
};

}  // namespace hams
