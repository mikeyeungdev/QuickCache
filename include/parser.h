#pragma once

#include <optional>
#include <string>

namespace quickcache {

enum class CommandType {
    Ping,
    Set,
    Get,
    Delete,
    Expire,
    Ttl,
    Stats,
    Invalid
};

struct Command {
    CommandType type = CommandType::Invalid;
    std::string key;
    std::string value;
    std::optional<int> ttl_seconds;
    std::string error;
};

class Parser {
public:
    Command parse(const std::string& line) const;
};

} // namespace quickcache
