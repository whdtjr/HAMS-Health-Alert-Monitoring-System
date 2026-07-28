#pragma once

#include "hams/assessment/assessment_context.hpp"

namespace hams {

class AssessmentRule {
public:
    virtual ~AssessmentRule() = default;
    [[nodiscard]] virtual bool matches(
        const AssessmentContext& context) const = 0;
};

}  // namespace hams
