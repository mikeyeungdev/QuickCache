#include "parser.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <sstream>

namespace quickcache {

Command Parser::parse(const std::string& line) const {
    Command command;
    const auto tokens = tokenize(line);
    if (tokens.empty()) {
        command.error = "ERR empty command";
        return command;
    }

    std::string op = tokens[0];
    std::transform(op.begin(), op.end(), op.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    try {
        if (op == "PING" && tokens.size() == 1) {
            command.type = CommandType::Ping;
        } else if (op == "SET" && (tokens.size() == 3 || tokens.size() == 5)) {
            command.type = CommandType::Set;
            command.key = tokens[1];
            command.value = tokens[2];
            if (tokens.size() == 5) {
                std::string ex = tokens[3];
                std::transform(ex.begin(), ex.end(), ex.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
                if (ex != "EX") {
                    command.type = CommandType::Unknown;
                    command.error = "ERR expected EX";
                    return command;
                }
                command.ttl_seconds = std::stoi(tokens[4]);
                if (*command.ttl_seconds < 0) {
                    command.type = CommandType::Unknown;
                    command.error = "ERR ttl must be non-negative";
                }
            }
        } else if (op == "GET" && tokens.size() == 2) {
            command.type = CommandType::Get;
            command.key = tokens[1];
        } else if ((op == "DELETE" || op == "DEL") && tokens.size() == 2) {
            command.type = CommandType::Delete;
            command.key = tokens[1];
        } else if (op == "EXPIRE" && tokens.size() == 3) {
            command.type = CommandType::Expire;
            command.key = tokens[1];
            command.ttl_seconds = std::stoi(tokens[2]);
            if (*command.ttl_seconds < 0) {
                command.type = CommandType::Unknown;
                command.error = "ERR ttl must be non-negative";
            }
        } else if (op == "TTL" && tokens.size() == 2) {
            command.type = CommandType::Ttl;
            command.key = tokens[1];
        } else if (op == "STATS" && tokens.size() == 1) {
            command.type = CommandType::Stats;
        } else {
            command.error = "ERR unknown or invalid command";
        }
    } catch (const std::exception&) {
        command.type = CommandType::Unknown;
        command.error = "ERR invalid integer";
    }

    return command;
}

std::vector<std::string> Parser::tokenize(const std::string& line) const {
    std::istringstream stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

} // namespace quickcache
