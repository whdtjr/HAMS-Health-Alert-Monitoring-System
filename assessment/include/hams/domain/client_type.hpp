#pragma once

#include <stdexcept>
#include <string_view>

namespace hams {

enum class ClientType {
    hrv,
    drowsiness,
    arrhythmia
};

inline ClientType clientTypeFromString(std::string_view value) {
    if (value == "hrv") {
        return ClientType::hrv;
    }
    if (value == "drowsy" || value == "drowsiness") {
        return ClientType::drowsiness;
    }
    if (value == "arrhythmia") {
        return ClientType::arrhythmia;
    }
    throw std::invalid_argument{"unknown client type"};
}

}  // namespace hams
