#pragma once

#include "hams/domain/alert.hpp"

namespace hams {

class AlertPublisher {
public:
    virtual ~AlertPublisher() = default;
    virtual void publish(AlertType alert) = 0;
};

}  // namespace hams
