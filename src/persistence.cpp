#include "persistence.h"

#include <cctype>
#include <chrono>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <cstdint>
#include <sstream>
#include <utility>
#include <vector>

#include "cache.h"

namespace quickcache {
namespace {

std::vector<std::string> split(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::int64_t now_epoch_seconds() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
}

} // namespace

PersistenceLog::PersistenceLog(std::string path) : path_(std::move(path)) {}

bool PersistenceLog::open() {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto parent = std::filesystem::path(path_).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    stream_.open(path_, std::ios::app);
    return stream_.is_open();
}

void PersistenceLog::append_set(const std::string& key, const std::string& value, std::optional<int> ttl_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    stream_ << "SET " << encode(key) << " " << encode(value);
    if (ttl_seconds.has_value()) {
        stream_ << " EXAT " << now_epoch_seconds() + *ttl_seconds;
    }
    stream_ << '\n';
    stream_.flush();
}

void PersistenceLog::append_delete(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    stream_ << "DELETE " << encode(key) << '\n';
    stream_.flush();
}

void PersistenceLog::append_expire(const std::string& key, int ttl_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    stream_ << "EXPIREAT " << encode(key) << " " << now_epoch_seconds() + ttl_seconds << '\n';
    stream_.flush();
}

void PersistenceLog::replay(Cache& cache) {
    std::ifstream input(path_);
    if (!input.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        try {
            const auto tokens = split(line);
            if (tokens.empty()) {
                continue;
            }

            if (tokens[0] == "SET" && (tokens.size() == 3 || tokens.size() == 5)) {
                std::optional<int> ttl_seconds = std::nullopt;
                if (tokens.size() == 5 && tokens[3] == "EXAT") {
                    const auto remaining = std::stoll(tokens[4]) - now_epoch_seconds();
                    if (remaining <= 0) {
                        cache.erase(decode(tokens[1]));
                        continue;
                    }
                    ttl_seconds = static_cast<int>(remaining);
                }
                cache.set(decode(tokens[1]), decode(tokens[2]), ttl_seconds);
            } else if (tokens[0] == "DELETE" && tokens.size() == 2) {
                cache.erase(decode(tokens[1]));
            } else if (tokens[0] == "EXPIREAT" && tokens.size() == 3) {
                const auto remaining = std::stoll(tokens[2]) - now_epoch_seconds();
                if (remaining <= 0) {
                    cache.erase(decode(tokens[1]));
                } else {
                    cache.expire(decode(tokens[1]), static_cast<int>(remaining));
                }
            }
        } catch (const std::exception&) {
            continue;
        }
    }
}

const std::string& PersistenceLog::path() const {
    return path_;
}

std::string PersistenceLog::encode(const std::string& value) {
    std::ostringstream out;
    for (unsigned char c : value) {
        if (std::isalnum(c) || c == ':' || c == '_' || c == '-' || c == '.') {
            out << c;
        } else {
            out << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c)
                << std::nouppercase << std::dec;
        }
    }
    return out.str();
}

std::string PersistenceLog::decode(const std::string& value) {
    std::string out;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const auto hex = value.substr(i + 1, 2);
            const auto decoded = static_cast<char>(std::stoi(hex, nullptr, 16));
            out.push_back(decoded);
            i += 2;
        } else {
            out.push_back(value[i]);
        }
    }
    return out;
}

} // namespace quickcache
