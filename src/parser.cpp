#include "parser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace quickcache {
namespace {

std::string uppercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> tokens;
    std::string token;

    while (input >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

std::optional<int> parse_ttl_seconds(const std::string& token) {
    try {
        std::size_t parsed_chars = 0;
        const int ttl = std::stoi(token, &parsed_chars);

        if (parsed_chars != token.size() || ttl < 0) {
            return std::nullopt;
        }

        return ttl;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

Command invalid_command(const std::string& error) {
    Command command;
    command.error = error;
    return command;
}

} // namespace

Command Parser::parse(const std::string& line) const {
    const auto tokens = tokenize(line);
    if (tokens.empty()) {
        return invalid_command("empty command");
    }

    const std::string command_name = uppercase(tokens[0]);

    if (command_name == "PING") {
        if (tokens.size() != 1) {
            return invalid_command("PING does not accept arguments");
        }
        return Command{CommandType::Ping};
    }

    if (command_name == "SET") {
        if (tokens.size() != 3 && tokens.size() != 5) {
            return invalid_command("SET requires: SET key value [EX seconds]");
        }

        Command command;
        command.type = CommandType::Set;
        command.key = tokens[1];
        command.value = tokens[2];

        if (command.key.empty()) {
            return invalid_command("SET requires a key");
        }

        if (tokens.size() == 5) {
            if (uppercase(tokens[3]) != "EX") {
                return invalid_command("SET optional TTL must use EX");
            }

            const auto ttl = parse_ttl_seconds(tokens[4]);
            if (!ttl.has_value()) {
                return invalid_command("TTL seconds must be a non-negative integer");
            }
            command.ttl_seconds = ttl;
        }

        return command;
    }

    if (command_name == "GET") {
        if (tokens.size() != 2) {
            return invalid_command("GET requires: GET key");
        }
        return Command{CommandType::Get, tokens[1]};
    }

    if (command_name == "DELETE") {
        if (tokens.size() != 2) {
            return invalid_command("DELETE requires: DELETE key");
        }
        return Command{CommandType::Delete, tokens[1]};
    }

    if (command_name == "EXPIRE") {
        if (tokens.size() != 3) {
            return invalid_command("EXPIRE requires: EXPIRE key seconds");
        }

        const auto ttl = parse_ttl_seconds(tokens[2]);
        if (!ttl.has_value()) {
            return invalid_command("TTL seconds must be a non-negative integer");
        }

        Command command;
        command.type = CommandType::Expire;
        command.key = tokens[1];
        command.ttl_seconds = ttl;
        return command;
    }

    if (command_name == "TTL") {
        if (tokens.size() != 2) {
            return invalid_command("TTL requires: TTL key");
        }
        return Command{CommandType::Ttl, tokens[1]};
    }

    if (command_name == "STATS") {
        if (tokens.size() != 1) {
            return invalid_command("STATS does not accept arguments");
        }
        return Command{CommandType::Stats};
    }

    return invalid_command("unknown command: " + tokens[0]);
}

} // namespace quickcache
