#pragma once

#include "hams/alert/alert_publisher.hpp"

#include <filesystem>

namespace hams {

class FifoAlertPublisher final : public AlertPublisher {
public:
    explicit FifoAlertPublisher(std::filesystem::path path);
    void publish(AlertType alert) override;

private:
    std::filesystem::path path_;
};

}  // namespace hams
