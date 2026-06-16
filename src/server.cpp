#include "server.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

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
using NativeSocket = SOCKET;
constexpr std::uintptr_t invalid_socket_value = INVALID_SOCKET;

void closeSocket(std::uintptr_t socket) {
    closesocket(static_cast<SOCKET>(socket));
}
#else
using NativeSocket = int;
constexpr int invalid_socket_value = -1;

void closeSocket(int socket) {
    close(socket);
}
#endif

NativeSocket nativeSocket(SocketHandle socket) {
    return static_cast<NativeSocket>(socket);
}

} // namespace

Server::Server(Cache& cache, int port) : cache_(cache), port_(port) {}

void Server::run() {
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        throw std::runtime_error("failed to initialize WinSock");
    }
#endif

    const SocketHandle server_socket = static_cast<SocketHandle>(::socket(AF_INET, SOCK_STREAM, 0));
    if (server_socket == invalid_socket_value) {
        throw std::runtime_error("failed to create server socket");
    }

    int enabled = 1;
    setsockopt(nativeSocket(server_socket), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&enabled), sizeof(enabled));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<unsigned short>(port_));

    if (bind(nativeSocket(server_socket), reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        closeSocket(server_socket);
        throw std::runtime_error("failed to bind server socket");
    }

    if (listen(nativeSocket(server_socket), 8) < 0) {
        closeSocket(server_socket);
        throw std::runtime_error("failed to listen on server socket");
    }

    std::cout << "QuickCache listening on port " << port_ << std::endl;

    while (true) {
        const SocketHandle client_socket = static_cast<SocketHandle>(accept(nativeSocket(server_socket), nullptr, nullptr));
        if (client_socket == invalid_socket_value) {
            std::cerr << "ERROR failed to accept client" << std::endl;
            continue;
        }

        std::thread([this, client_socket]() {
            handleClient(client_socket);
            closeSocket(client_socket);
        }).detach();
    }
}

void Server::handleClient(SocketHandle client_socket) {
    std::string pending;
    char buffer[4096];

    while (true) {
        const int received = recv(nativeSocket(client_socket), buffer, sizeof(buffer), 0);
        if (received <= 0) {
            return;
        }

        pending.append(buffer, buffer + received);

        std::size_t newline = std::string::npos;
        while ((newline = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, newline);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            pending.erase(0, newline + 1);

            if (!sendLine(client_socket, execute(parser_.parse(line)))) {
                return;
            }
        }
    }
}

std::string Server::execute(const Command& command) {
    if (command.type == CommandType::Invalid) {
        return "ERROR " + command.error;
    }

    switch (command.type) {
    case CommandType::Ping:
        return "PONG";
    case CommandType::Set: {
        const auto status = command.ttl_seconds.has_value()
            ? cache_.set(command.key, command.value, command.ttl_seconds.value())
            : cache_.set(command.key, command.value);
        return status == CacheStatus::Ok ? "OK" : "ERROR failed to set key";
    }
    case CommandType::Get: {
        const auto value = cache_.get(command.key);
        return value.has_value() ? "VALUE " + value.value() : "NOT_FOUND";
    }
    case CommandType::Delete:
        return cache_.remove(command.key) == CacheStatus::Ok ? "OK" : "NOT_FOUND";
    case CommandType::Expire: {
        const auto status = cache_.expire(command.key, command.ttl_seconds.value());
        if (status == CacheStatus::Ok) {
            return "OK";
        }
        return status == CacheStatus::NotFound ? "NOT_FOUND" : "ERROR failed to expire key";
    }
    case CommandType::Ttl: {
        const auto ttl = cache_.ttl(command.key);
        if (ttl.has_value()) {
            return "TTL " + std::to_string(ttl.value());
        }

        if (cache_.get(command.key).has_value()) {
            return "TTL -1";
        }
        return "NOT_FOUND";
    }
    case CommandType::Stats: {
        std::ostringstream output;
        output << "OK keys=" << cache_.size();
        return output.str();
    }
    case CommandType::Invalid:
        break;
    }

    return "ERROR unsupported command";
}

bool Server::sendLine(SocketHandle client_socket, const std::string& line) {
    const std::string response = line + "\n";
    const char* data = response.data();
    std::size_t remaining = response.size();

    while (remaining > 0) {
        const int sent = send(nativeSocket(client_socket), data, static_cast<int>(remaining), 0);
        if (sent <= 0) {
            return false;
        }
        data += sent;
        remaining -= static_cast<std::size_t>(sent);
    }

    return true;
}

} // namespace quickcache
