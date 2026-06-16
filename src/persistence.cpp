#include "persistence.h"

#include <chrono>
#include <cctype>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

#include "cache.h"

namespace quickcache {
namespace {

std::vector<std::string> split(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> tokens;
    std::string token;

    while (input >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

} // namespace

AppendOnlyLog::AppendOnlyLog(std::string path) : path_(std::move(path)) {}

bool AppendOnlyLog::replay(Cache& cache) const {
    std::ifstream input(path_);
    if (!input.is_open()) {
        return true;
    }

    std::string line;
    while (std::getline(input, line)) {
        const auto tokens = split(line);
        if (tokens.empty()) {
            continue;
        }

        try {
            if (tokens[0] == "SET" && (tokens.size() == 3 || tokens.size() == 5)) {
                const auto key = decode(tokens[1]);
                const auto value = decode(tokens[2]);

                if (tokens.size() == 5 && tokens[3] == "EXAT") {
                    const auto remaining = std::stoll(tokens[4]) - currentEpochSeconds();
                    if (remaining <= 0) {
                        cache.remove(key);
                    } else {
                        cache.set(key, value, static_cast<int>(remaining));
                    }
                } else {
                    cache.set(key, value);
                }
            } else if (tokens[0] == "DELETE" && tokens.size() == 2) {
                cache.remove(decode(tokens[1]));
            } else if (tokens[0] == "EXPIREAT" && tokens.size() == 3) {
                const auto key = decode(tokens[1]);
                const auto remaining = std::stoll(tokens[2]) - currentEpochSeconds();
                if (remaining <= 0) {
                    cache.remove(key);
                } else {
                    cache.expire(key, static_cast<int>(remaining));
                }
            }
        } catch (const std::exception&) {
            continue;
        }
    }

    return true;
}

bool AppendOnlyLog::openForAppend() {
    const auto parent = std::filesystem::path(path_).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    output_.open(path_, std::ios::app);
    return output_.is_open();
}

void AppendOnlyLog::appendSet(const std::string& key, const std::string& value) {
    appendLine("SET " + encode(key) + " " + encode(value));
}

void AppendOnlyLog::appendSet(const std::string& key, const std::string& value, int ttl_seconds) {
    appendLine(
        "SET " + encode(key) + " " + encode(value) + " EXAT " + std::to_string(currentEpochSeconds() + ttl_seconds));
}

void AppendOnlyLog::appendDelete(const std::string& key) {
    appendLine("DELETE " + encode(key));
}

void AppendOnlyLog::appendExpire(const std::string& key, int ttl_seconds) {
    appendLine("EXPIREAT " + encode(key) + " " + std::to_string(currentEpochSeconds() + ttl_seconds));
}

const std::string& AppendOnlyLog::path() const {
    return path_;
}

std::string AppendOnlyLog::encode(const std::string& value) {
    std::ostringstream output;

    for (const unsigned char ch : value) {
        if (std::isalnum(ch) || ch == ':' || ch == '_' || ch == '-' || ch == '.') {
            output << ch;
        } else {
            output << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<int>(ch) << std::nouppercase << std::dec;
        }
    }

    return output.str();
}

std::string AppendOnlyLog::decode(const std::string& value) {
    std::string output;

    for (std::size_t index = 0; index < value.size(); ++index) {
        if (value[index] == '%' && index + 2 < value.size()) {
            output.push_back(static_cast<char>(std::stoi(value.substr(index + 1, 2), nullptr, 16)));
            index += 2;
        } else {
            output.push_back(value[index]);
        }
    }

    return output;
}

long long AppendOnlyLog::currentEpochSeconds() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
}

void AppendOnlyLog::appendLine(const std::string& line) {
    std::lock_guard<std::mutex> lock(mutex_);
    output_ << line << '\n';
    output_.flush();
}

} // namespace quickcache
