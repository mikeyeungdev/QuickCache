#include "parser.h"

#include <cassert>
#include <iostream>

namespace {

void test_valid_ping() {
    quickcache::Parser parser;
    const auto command = parser.parse("PING");

    assert(command.type == quickcache::CommandType::Ping);
    assert(command.error.empty());
}

void test_valid_set() {
    quickcache::Parser parser;
    const auto command = parser.parse("SET code:user123 849201");

    assert(command.type == quickcache::CommandType::Set);
    assert(command.key == "code:user123");
    assert(command.value == "849201");
    assert(!command.ttl_seconds.has_value());
    assert(command.error.empty());
}

void test_valid_set_with_ex() {
    quickcache::Parser parser;
    const auto command = parser.parse("SET code:user123 849201 EX 300");

    assert(command.type == quickcache::CommandType::Set);
    assert(command.key == "code:user123");
    assert(command.value == "849201");
    assert(command.ttl_seconds.has_value());
    assert(command.ttl_seconds.value() == 300);
    assert(command.error.empty());
}

void test_valid_get() {
    quickcache::Parser parser;
    const auto command = parser.parse("GET code:user123");

    assert(command.type == quickcache::CommandType::Get);
    assert(command.key == "code:user123");
    assert(command.error.empty());
}

void test_valid_delete() {
    quickcache::Parser parser;
    const auto command = parser.parse("DELETE code:user123");

    assert(command.type == quickcache::CommandType::Delete);
    assert(command.key == "code:user123");
    assert(command.error.empty());
}

void test_invalid_missing_key() {
    quickcache::Parser parser;
    const auto command = parser.parse("GET");

    assert(command.type == quickcache::CommandType::Invalid);
    assert(!command.error.empty());
}

void test_invalid_ttl_value() {
    quickcache::Parser parser;
    const auto command = parser.parse("SET code:user123 849201 EX soon");

    assert(command.type == quickcache::CommandType::Invalid);
    assert(!command.error.empty());
}

void test_unknown_command() {
    quickcache::Parser parser;
    const auto command = parser.parse("FETCH code:user123");

    assert(command.type == quickcache::CommandType::Invalid);
    assert(!command.error.empty());
}

void test_valid_expire_ttl_and_stats() {
    quickcache::Parser parser;

    const auto expire = parser.parse("EXPIRE code:user123 300");
    assert(expire.type == quickcache::CommandType::Expire);
    assert(expire.key == "code:user123");
    assert(expire.ttl_seconds.has_value());
    assert(expire.ttl_seconds.value() == 300);

    const auto ttl = parser.parse("TTL code:user123");
    assert(ttl.type == quickcache::CommandType::Ttl);
    assert(ttl.key == "code:user123");

    const auto stats = parser.parse("STATS");
    assert(stats.type == quickcache::CommandType::Stats);
}

} // namespace

int main() {
    test_valid_ping();
    test_valid_set();
    test_valid_set_with_ex();
    test_valid_get();
    test_valid_delete();
    test_invalid_missing_key();
    test_invalid_ttl_value();
    test_unknown_command();
    test_valid_expire_ttl_and_stats();

    std::cout << "parser_tests passed" << std::endl;
    return 0;
}
