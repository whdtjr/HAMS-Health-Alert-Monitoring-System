#pragma once

namespace hams {

class Socket {
public:
    Socket() = default;
    explicit Socket(int descriptor) noexcept;
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    [[nodiscard]] int nativeHandle() const noexcept;
    [[nodiscard]] bool valid() const noexcept;
    void close() noexcept;

private:
    int descriptor_{-1};
};

}  // namespace hams
