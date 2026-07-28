#pragma once

#include "hams/domain/client_type.hpp"
#include "hams/domain/ppg_data.hpp"

#include <string_view>

namespace hams {

class MessageParser {
public:
    virtual ~MessageParser() = default;

    [[nodiscard]] virtual ClientType parseClientType(
        std::string_view message) const = 0;

    [[nodiscard]] virtual PpgData parsePpgData(
        std::string_view message) const = 0;
};

}  // namespace hams
