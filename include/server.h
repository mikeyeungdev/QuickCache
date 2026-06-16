#pragma once

#include <cstdint>
#include <string>

#include "cache.h"
#include "persistence.h"
#include "parser.h"

namespace quickcache {

using SocketHandle =
#ifdef _WIN32
    std::uintptr_t;
#else
    int;
#endif

class Server {
public:
    Server(Cache& cache, AppendOnlyLog* append_only_log = nullptr, int port = 6379);
    void run();

private:
    void handleClient(SocketHandle client_socket);
    std::string execute(const Command& command);
    bool sendLine(SocketHandle client_socket, const std::string& line);

    Cache& cache_;
    AppendOnlyLog* append_only_log_;
    int port_;
    Parser parser_;
};

} // namespace quickcache
