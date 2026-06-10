#pragma once

#include <optional>
#include <string>
#include <vector>

namespace quickcache {

enum class CommandType {
    Ping,
    Set,
    Get,
    Delete,
    Expire,
    Ttl,
    Stats,
    Unknown
};

struct Command {
    CommandType type = CommandType::Unknown;
    std::string key;
    std::string value;
    std::optional<int> ttl_seconds;
    std::string error;
};

class Parser {
public:
    Command parse(const std::string& line) const;

private:
    std::vector<std::string> tokenize(const std::string& line) const;
};

} // namespace quickcache
