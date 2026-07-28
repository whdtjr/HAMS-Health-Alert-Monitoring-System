#include "hams/net/client_session.hpp"

#include <array>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <sys/socket.h>

namespace hams {
namespace {

std::optional<std::size_t> completeJsonEnd(std::string_view input) {
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    bool started = false;

    for (std::size_t index = 0; index < input.size(); ++index) {
        const char current = input[index];
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (current == '\\') {
                escaped = true;
            } else if (current == '"') {
                inString = false;
            }
            continue;
        }

        if (current == '"') {
            inString = true;
        } else if (current == '{') {
            ++depth;
            started = true;
        } else if (current == '}') {
            --depth;
            if (started && depth == 0) {
                return index + 1;
            }
            if (depth < 0) {
                throw std::runtime_error{"invalid JSON message boundary"};
            }
        }
    }
    return std::nullopt;
}

}  // namespace

ClientSession::ClientSession(Socket socket)
    : socket_{std::move(socket)} {}

std::optional<std::string> ClientSession::extractMessage() {
    const auto lineEnd = pending_.find('\n');
    const auto jsonEnd = completeJsonEnd(pending_);

    std::optional<std::size_t> end;
    if (lineEnd != std::string::npos) {
        end = lineEnd;
    }
    if (jsonEnd && (!end || *jsonEnd < *end)) {
        end = jsonEnd;
    }
    if (!end) {
        return std::nullopt;
    }

    std::string message = pending_.substr(0, *end);
    std::size_t consumed = *end;
    if (consumed < pending_.size() && pending_[consumed] == '\n') {
        ++consumed;
    }
    pending_.erase(0, consumed);
    return message;
}

std::optional<std::string> ClientSession::receiveMessage() {
    constexpr std::size_t maximumMessageSize = 16 * 1024;
    std::array<char, 2048> buffer{};

    while (true) {
        if (auto message = extractMessage()) {
            return message;
        }
        if (pending_.size() >= maximumMessageSize) {
            throw std::length_error{"client message exceeds size limit"};
        }

        const auto count = ::recv(
            socket_.nativeHandle(),
            buffer.data(),
            buffer.size(),
            0);

        if (count == 0) {
            return std::nullopt;
        }
        if (count < 0) {
            throw std::runtime_error{"socket receive failed"};
        }
        pending_.append(buffer.data(), static_cast<std::size_t>(count));
    }
}

}  // namespace hams
