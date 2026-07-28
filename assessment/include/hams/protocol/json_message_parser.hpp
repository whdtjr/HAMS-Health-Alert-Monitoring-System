#pragma once

#include "hams/protocol/message_parser.hpp"

namespace hams {

class JsonMessageParser final : public MessageParser {
public:
    [[nodiscard]] ClientType parseClientType(
        std::string_view message) const override;

    [[nodiscard]] PpgData parsePpgData(
        std::string_view message) const override;
};

}  // namespace hams
