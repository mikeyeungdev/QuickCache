#include "server.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace quickcache {
namespace {

#ifdef _WIN32
constexpr std::uintptr_t invalid_socket_value = INVALID_SOCKET;
using NativeSocket = SOCKET;
void close_socket(std::uintptr_t socket) {
    closesocket(static_cast<SOCKET>(socket));
}
#else
constexpr int invalid_socket_value = -1;
using NativeSocket = int;
void close_socket(int socket) {
    close(socket);
}
#endif

NativeSocket native_socket(Server::SocketHandle socket) {
    return static_cast<NativeSocket>(socket);
}

} // namespace

Server::Server(std::string host, int port, Cache& cache, PersistenceLog& log)
    : host_(std::move(host)), port_(port), cache_(cache), log_(log), listen_socket_(invalid_socket_value) {}

Server::~Server() {
    stop();
}

void Server::run() {
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    listen_socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_ == invalid_socket_value) {
        throw std::runtime_error("socket creation failed");
    }

    int enabled = 1;
    setsockopt(native_socket(listen_socket_), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&enabled), sizeof(enabled));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port_));
    if (inet_pton(AF_INET, host_.c_str(), &address.sin_addr) != 1) {
        throw std::runtime_error("invalid bind host");
    }

    if (bind(native_socket(listen_socket_), reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error("bind failed");
    }

    if (listen(native_socket(listen_socket_), SOMAXCONN) < 0) {
        throw std::runtime_error("listen failed");
    }

    running_ = true;
    std::cout << "QuickCache listening on " << host_ << ":" << port_ << '\n';

    while (running_) {
        auto client = accept(native_socket(listen_socket_), nullptr, nullptr);
        if (client == invalid_socket_value) {
            if (running_) {
                std::cerr << "accept failed\n";
            }
            continue;
        }
        client_threads_.emplace_back(&Server::handle_client, this, static_cast<SocketHandle>(client));
    }
}

void Server::stop() {
    running_ = false;
    if (listen_socket_ != invalid_socket_value) {
        close_socket(listen_socket_);
        listen_socket_ = invalid_socket_value;
    }

    for (auto& thread : client_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

std::string Server::execute(const Command& command) {
    if (command.type == CommandType::Unknown) {
        return command.error.empty() ? "ERR unknown command" : command.error;
    }

    switch (command.type) {
    case CommandType::Ping:
        return "PONG";
    case CommandType::Set:
        cache_.set(command.key, command.value, command.ttl_seconds);
        log_.append_set(command.key, command.value, command.ttl_seconds);
        return "OK";
    case CommandType::Get: {
        auto value = cache_.get(command.key);
        return value.has_value() ? *value : "(nil)";
    }
    case CommandType::Delete: {
        const bool deleted = cache_.erase(command.key);
        if (deleted) {
            log_.append_delete(command.key);
        }
        return deleted ? "1" : "0";
    }
    case CommandType::Expire: {
        const bool changed = cache_.expire(command.key, *command.ttl_seconds);
        if (changed) {
            log_.append_expire(command.key, *command.ttl_seconds);
        }
        return changed ? "1" : "0";
    }
    case CommandType::Ttl:
        return std::to_string(cache_.ttl(command.key));
    case CommandType::Stats:
        return stats_response();
    case CommandType::Unknown:
        break;
    }
    return "ERR unknown command";
}

void Server::handle_client(SocketHandle client) {
    std::string pending;
    char buffer[4096];

    while (running_) {
        const int received = recv(native_socket(client), buffer, sizeof(buffer), 0);
        if (received <= 0) {
            break;
        }

        pending.append(buffer, buffer + received);
        std::size_t newline = std::string::npos;
        while ((newline = pending.find('\n')) != std::string::npos) {
            auto line = pending.substr(0, newline);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            pending.erase(0, newline + 1);
            const auto response = execute(parser_.parse(line));
            if (!send_line(client, response)) {
                close_socket(client);
                return;
            }
        }
    }

    close_socket(client);
}

bool Server::send_line(SocketHandle client, const std::string& response) {
    const std::string wire = response + "\n";
    const char* data = wire.data();
    std::size_t remaining = wire.size();
    while (remaining > 0) {
        const int sent = send(native_socket(client), data, static_cast<int>(remaining), 0);
        if (sent <= 0) {
            return false;
        }
        data += sent;
        remaining -= static_cast<std::size_t>(sent);
    }
    return true;
}

std::string Server::stats_response() {
    const auto snapshot = cache_.stats();
    std::ostringstream out;
    out << "keys=" << snapshot.keys
        << " hits=" << snapshot.hits
        << " misses=" << snapshot.misses
        << " sets=" << snapshot.sets
        << " deletes=" << snapshot.deletes
        << " expirations=" << snapshot.expirations
        << " evictions=" << snapshot.evictions
        << " commands=" << snapshot.commands;
    return out.str();
}

} // namespace quickcache
