#pragma once

#include "hams/concurrency/blocking_queue.hpp"
#include "hams/domain/ppg_data.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace hams {

class PpgRepository {
public:
    explicit PpgRepository(std::size_t capacity) : samples_{capacity} {}

    void save(PpgData data) {
        samples_.pushOverwrite(std::move(data));
    }

    [[nodiscard]] std::vector<PpgData> snapshot() const {
        return samples_.snapshot();
    }

    [[nodiscard]] std::size_t size() const {
        return samples_.size();
    }

private:
    BlockingQueue<PpgData> samples_;
};

}  // namespace hams
