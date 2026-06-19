#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle invalid_socket = INVALID_SOCKET;

void closeSocket(SocketHandle socket) {
    closesocket(socket);
}
#else
using SocketHandle = int;
constexpr SocketHandle invalid_socket = -1;

void closeSocket(SocketHandle socket) {
    close(socket);
}
#endif

struct Options {
    std::string host = "127.0.0.1";
    int port = 6379;
    int clients = 50;
    int requests = 10000;
    int pipeline = 1;
    std::string workload = "verification_codes";
};

struct WorkerResult {
    std::vector<double> latencies_ms;
    int successful = 0;
    int errors = 0;
};

std::string valueFor(const std::string& name, int argc, char** argv, int& index) {
    if (index + 1 >= argc) {
        throw std::runtime_error("missing value for " + name);
    }
    ++index;
    return argv[index];
}

Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--host") {
            options.host = valueFor(arg, argc, argv, i);
        } else if (arg == "--port") {
            options.port = std::stoi(valueFor(arg, argc, argv, i));
        } else if (arg == "--clients") {
            options.clients = std::stoi(valueFor(arg, argc, argv, i));
        } else if (arg == "--requests") {
            options.requests = std::stoi(valueFor(arg, argc, argv, i));
        } else if (arg == "--pipeline") {
            options.pipeline = std::stoi(valueFor(arg, argc, argv, i));
        } else if (arg == "--workload") {
            options.workload = valueFor(arg, argc, argv, i);
        } else {
            throw std::runtime_error("unknown option: " + arg);
        }
    }

    if (options.clients <= 0 || options.requests <= 0 || options.pipeline <= 0) {
        throw std::runtime_error("--clients, --requests, and --pipeline must be positive");
    }
    return options;
}

std::string commandFor(const std::string& workload, int request_id) {
    const int user_id = request_id % 100000;

    if (workload == "ping") {
        return "PING\n";
    }

    if (workload == "get_missing") {
        return "GET missing:user" + std::to_string(user_id) + "\n";
    }

    if (workload == "verification_codes") {
        if (request_id % 2 == 0) {
            const int code = 100000 + (request_id % 900000);
            return "SET code:user" + std::to_string(user_id) + " " + std::to_string(code) + " EX 300\n";
        }
        return "GET code:user" + std::to_string(user_id) + "\n";
    }

    if (workload == "ticket_queue") {
        if (request_id % 4 == 0 || request_id % 4 == 1) {
            const int position = 1 + (request_id % 50000);
            return "SET queue:user" + std::to_string(user_id) + " " + std::to_string(position) + " EX 1800\n";
        }
        return "GET queue:user" + std::to_string(user_id) + "\n";
    }

    if (workload == "inventory_holds") {
        const int item_id = request_id % 10000;
        const std::string key = "hold:item" + std::to_string(item_id) + ":user" + std::to_string(user_id);
        if (request_id % 3 == 0) {
            return "SET " + key + " reserved EX 600\n";
        }
        if (request_id % 3 == 1) {
            return "GET " + key + "\n";
        }
        return "DELETE " + key + "\n";
    }

    throw std::runtime_error("unknown workload: " + workload);
}

bool isSuccess(const std::string& response) {
    return response == "OK" || response == "PONG" || response == "NOT_FOUND" ||
           response.rfind("VALUE ", 0) == 0 || response.rfind("TTL ", 0) == 0;
}

SocketHandle connectToServer(const std::string& host, int port) {
    for (int attempt = 0; attempt < 50; ++attempt) {
        SocketHandle socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket_handle == invalid_socket) {
            throw std::runtime_error("failed to create socket");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<unsigned short>(port));

        if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
            closeSocket(socket_handle);
            throw std::runtime_error("benchmark_client.cpp currently expects an IPv4 host such as 127.0.0.1");
        }

        if (connect(socket_handle, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) {
            return socket_handle;
        }

        closeSocket(socket_handle);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    throw std::runtime_error("failed to connect");
}

void sendAll(SocketHandle socket_handle, const std::string& data) {
    const char* cursor = data.data();
    std::size_t remaining = data.size();
    while (remaining > 0) {
        const int sent = send(socket_handle, cursor, static_cast<int>(remaining), 0);
        if (sent <= 0) {
            throw std::runtime_error("send failed");
        }
        cursor += sent;
        remaining -= static_cast<std::size_t>(sent);
    }
}

WorkerResult runWorker(const Options& options, int start_id, int count) {
    WorkerResult result;
    result.latencies_ms.reserve(count);

    try {
        const SocketHandle socket_handle = connectToServer(options.host, options.port);
        std::vector<std::chrono::steady_clock::time_point> starts;
        starts.reserve(options.pipeline);
        std::string receive_buffer;
        std::string outbound;
        char buffer[8192];
        int sent = 0;
        int received = 0;
        int start_cursor = 0;

        while (received < count) {
            outbound.clear();
            while (sent < count && static_cast<int>(starts.size()) - start_cursor < options.pipeline) {
                starts.push_back(std::chrono::steady_clock::now());
                outbound += commandFor(options.workload, start_id + sent);
                ++sent;
            }

            if (!outbound.empty()) {
                sendAll(socket_handle, outbound);
            }

            while (receive_buffer.find('\n') == std::string::npos) {
                const int bytes = recv(socket_handle, buffer, sizeof(buffer), 0);
                if (bytes <= 0) {
                    throw std::runtime_error("recv failed");
                }
                receive_buffer.append(buffer, buffer + bytes);
            }

            const auto newline = receive_buffer.find('\n');
            std::string response = receive_buffer.substr(0, newline);
            if (!response.empty() && response.back() == '\r') {
                response.pop_back();
            }
            receive_buffer.erase(0, newline + 1);

            const auto elapsed = std::chrono::steady_clock::now() - starts[start_cursor++];
            result.latencies_ms.push_back(std::chrono::duration<double, std::milli>(elapsed).count());
            ++received;

            if (isSuccess(response)) {
                ++result.successful;
            } else {
                ++result.errors;
            }

            if (start_cursor > 1024 && start_cursor * 2 > static_cast<int>(starts.size())) {
                starts.erase(starts.begin(), starts.begin() + start_cursor);
                start_cursor = 0;
            }
        }

        closeSocket(socket_handle);
    } catch (...) {
        result.errors += count;
    }

    return result;
}

double percentile(const std::vector<double>& sorted_values, double percentile_value) {
    if (sorted_values.empty()) {
        return 0.0;
    }
    const auto index = static_cast<std::size_t>((percentile_value / 100.0) * (sorted_values.size() - 1));
    return sorted_values[index];
}

} // namespace

int main(int argc, char** argv) {
    try {
#ifdef _WIN32
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("failed to initialize WinSock");
        }
#endif

        const Options options = parseOptions(argc, argv);
        std::vector<std::thread> threads;
        std::vector<WorkerResult> results(options.clients);

        const auto started = std::chrono::steady_clock::now();
        int next_request_id = 0;
        for (int client = 0; client < options.clients; ++client) {
            const int request_count = options.requests / options.clients + (client < options.requests % options.clients ? 1 : 0);
            const int request_start = next_request_id;
            next_request_id += request_count;
            threads.emplace_back([&, client, request_start, request_count]() {
                results[client] = runWorker(options, request_start, request_count);
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        const auto elapsed = std::chrono::steady_clock::now() - started;
        const double elapsed_seconds = std::chrono::duration<double>(elapsed).count();

        std::vector<double> latencies;
        int successful = 0;
        int errors = 0;
        for (const auto& result : results) {
            latencies.insert(latencies.end(), result.latencies_ms.begin(), result.latencies_ms.end());
            successful += result.successful;
            errors += result.errors;
        }

        std::sort(latencies.begin(), latencies.end());
        const double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
        const double average = latencies.empty() ? 0.0 : sum / static_cast<double>(latencies.size());

        std::cout << "QuickCache C++ Benchmark Results\n";
        std::cout << "================================\n";
        std::cout << "workload: " << options.workload << "\n";
        std::cout << "host: " << options.host << "\n";
        std::cout << "port: " << options.port << "\n";
        std::cout << "clients: " << options.clients << "\n";
        std::cout << "pipeline: " << options.pipeline << "\n";
        std::cout << "total_requests: " << options.requests << "\n";
        std::cout << "successful_requests: " << successful << "\n";
        std::cout << "errors: " << errors << "\n";
        std::cout << "elapsed_seconds: " << elapsed_seconds << "\n";
        std::cout << "requests_per_second: " << options.requests / elapsed_seconds << "\n";
        std::cout << "latency_avg_ms: " << average << "\n";
        std::cout << "latency_p50_ms: " << percentile(latencies, 50) << "\n";
        std::cout << "latency_p95_ms: " << percentile(latencies, 95) << "\n";
        std::cout << "latency_p99_ms: " << percentile(latencies, 99) << "\n";

#ifdef _WIN32
        WSACleanup();
#endif
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR " << error.what() << "\n";
        return 1;
    }
}
