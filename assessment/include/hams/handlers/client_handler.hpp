#pragma once

namespace hams {

class ClientHandler {
public:
    virtual ~ClientHandler() = default;
    virtual void handle() = 0;
};

}  // namespace hams
