#include "cache.h"

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using quickcache::Cache;

TEST(CacheTest, SetAndGetValue) {
    Cache cache(10);
    cache.set("code:user123", "849201");

    EXPECT_EQ(cache.get("code:user123"), "849201");
    EXPECT_EQ(cache.ttl("code:user123"), -1);
}

TEST(CacheTest, ExpiresValues) {
    Cache cache(10);
    cache.set("code:user123", "849201", 0);

    EXPECT_EQ(cache.get("code:user123"), std::nullopt);
    EXPECT_EQ(cache.ttl("code:user123"), -2);
}

TEST(CacheTest, EvictsLeastRecentlyUsedKey) {
    Cache cache(2);
    cache.set("a", "1");
    cache.set("b", "2");
    EXPECT_EQ(cache.get("a"), "1");
    cache.set("c", "3");

    EXPECT_EQ(cache.get("b"), std::nullopt);
    EXPECT_EQ(cache.get("a"), "1");
    EXPECT_EQ(cache.get("c"), "3");
    EXPECT_EQ(cache.stats().evictions, 1);
}

TEST(CacheTest, DeleteRemovesValue) {
    Cache cache(10);
    cache.set("hold:item456:user123", "reserved");

    EXPECT_TRUE(cache.erase("hold:item456:user123"));
    EXPECT_FALSE(cache.erase("hold:item456:user123"));
    EXPECT_EQ(cache.get("hold:item456:user123"), std::nullopt);
}
