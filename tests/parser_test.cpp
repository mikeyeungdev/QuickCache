#include "parser.h"

#include <gtest/gtest.h>

using quickcache::CommandType;
using quickcache::Parser;

TEST(ParserTest, ParsesPing) {
    Parser parser;
    const auto command = parser.parse("PING");

    EXPECT_EQ(command.type, CommandType::Ping);
}

TEST(ParserTest, ParsesSetWithTtl) {
    Parser parser;
    const auto command = parser.parse("SET code:user123 849201 EX 300");

    EXPECT_EQ(command.type, CommandType::Set);
    EXPECT_EQ(command.key, "code:user123");
    EXPECT_EQ(command.value, "849201");
    ASSERT_TRUE(command.ttl_seconds.has_value());
    EXPECT_EQ(*command.ttl_seconds, 300);
}

TEST(ParserTest, RejectsInvalidTtl) {
    Parser parser;
    const auto command = parser.parse("EXPIRE code:user123 soon");

    EXPECT_EQ(command.type, CommandType::Unknown);
    EXPECT_EQ(command.error, "ERR invalid integer");
}

TEST(ParserTest, ParsesStats) {
    Parser parser;
    const auto command = parser.parse("STATS");

    EXPECT_EQ(command.type, CommandType::Stats);
}
