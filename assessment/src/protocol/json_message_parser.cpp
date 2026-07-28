#include "hams/protocol/json_message_parser.hpp"

#include <charconv>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <system_error>

namespace hams {
namespace {

std::size_t valuePosition(std::string_view json, std::string_view key) {
    const std::string quotedKey = "\"" + std::string{key} + "\"";
    const auto keyPosition = json.find(quotedKey);
    if (keyPosition == std::string_view::npos) {
        throw std::invalid_argument{"missing JSON field: " + std::string{key}};
    }

    const auto colon = json.find(':', keyPosition + quotedKey.size());
    if (colon == std::string_view::npos) {
        throw std::invalid_argument{"invalid JSON field: " + std::string{key}};
    }

    auto position = colon + 1;
    while (position < json.size() &&
           std::isspace(static_cast<unsigned char>(json[position]))) {
        ++position;
    }
    return position;
}

std::string stringField(std::string_view json, std::string_view key) {
    auto position = valuePosition(json, key);
    if (position >= json.size() || json[position] != '"') {
        throw std::invalid_argument{"JSON field is not a string: " +
                                    std::string{key}};
    }

    ++position;
    std::string value;
    bool escaped = false;
    for (; position < json.size(); ++position) {
        const char current = json[position];
        if (escaped) {
            value.push_back(current);
            escaped = false;
        } else if (current == '\\') {
            escaped = true;
        } else if (current == '"') {
            return value;
        } else {
            value.push_back(current);
        }
    }
    throw std::invalid_argument{"unterminated JSON string: " + std::string{key}};
}

double numberField(std::string_view json, std::string_view key) {
    const auto begin = valuePosition(json, key);
    auto end = begin;
    while (end < json.size()) {
        const char current = json[end];
        if (!(std::isdigit(static_cast<unsigned char>(current)) ||
              current == '-' || current == '+' ||
              current == '.' || current == 'e' || current == 'E')) {
            break;
        }
        ++end;
    }

    if (begin == end) {
        throw std::invalid_argument{"JSON field is not numeric: " +
                                    std::string{key}};
    }

    double value{};
    const auto result = std::from_chars(
        json.data() + begin,
        json.data() + end,
        value);

    if (result.ec != std::errc{} || result.ptr != json.data() + end ||
        !std::isfinite(value)) {
        throw std::invalid_argument{"invalid numeric JSON field: " +
                                    std::string{key}};
    }
    return value;
}

}  // namespace

ClientType JsonMessageParser::parseClientType(
    std::string_view message) const
{
    return clientTypeFromString(stringField(message, "id"));
}

PpgData JsonMessageParser::parsePpgData(
    std::string_view message) const
{
    const auto seconds = numberField(message, "timestamp");
    return {
        std::chrono::system_clock::time_point{
            std::chrono::milliseconds{
                static_cast<std::int64_t>(seconds * 1000.0)}},
        static_cast<int>(numberField(message, "signal")),
        static_cast<int>(numberField(message, "bpm")),
        static_cast<int>(numberField(message, "ibi")),
        numberField(message, "sdnn"),
        numberField(message, "rmssd"),
        numberField(message, "pnn50")
    };
}

}  // namespace hams
