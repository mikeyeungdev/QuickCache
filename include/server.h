#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "cache.h"
#include "parser.h"
#include "persistence.h"

namespace quickcache {

using SocketHandle =
#ifdef _WIN32
    std::uintptr_t;
#else
    int;
#endif

class Server {
public:
    Server(std::string host, int port, Cache& cache, PersistenceLog& log);
    ~Server();

    void run();
    void stop();

private:
    std::string execute(const Command& command);
    void handle_client(SocketHandle client);
    bool send_line(SocketHandle client, const std::string& response);
    std::string stats_response();

    std::string host_;
    int port_;
    Cache& cache_;
    PersistenceLog& log_;
    Parser parser_;
    std::atomic<bool> running_{false};
    SocketHandle listen_socket_{};
    std::vector<std::thread> client_threads_;
};

} // namespace quickcache
