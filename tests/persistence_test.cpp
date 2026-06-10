#include "cache.h"
#include "persistence.h"

#include <cstdio>
#include <string>

#include <gtest/gtest.h>

using quickcache::Cache;
using quickcache::PersistenceLog;

TEST(PersistenceTest, ReplaysAppendOnlyLog) {
    const std::string path = "quickcache_persistence_test.aof";
    std::remove(path.c_str());

    {
        PersistenceLog log(path);
        ASSERT_TRUE(log.open());
        log.append_set("code:user123", "849201", 300);
        log.append_set("queue:user123", "482", std::nullopt);
        log.append_delete("queue:user123");
    }

    Cache recovered(10);
    PersistenceLog replay(path);
    replay.replay(recovered);

    EXPECT_EQ(recovered.get("code:user123"), "849201");
    EXPECT_EQ(recovered.get("queue:user123"), std::nullopt);

    std::remove(path.c_str());
}

TEST(PersistenceTest, EncodesValuesWithSpaces) {
    const std::string path = "quickcache_persistence_space_test.aof";
    std::remove(path.c_str());

    {
        PersistenceLog log(path);
        ASSERT_TRUE(log.open());
        log.append_set("hold:item456:user123", "reserved until checkout", 600);
    }

    Cache recovered(10);
    PersistenceLog replay(path);
    replay.replay(recovered);

    EXPECT_EQ(recovered.get("hold:item456:user123"), "reserved until checkout");

    std::remove(path.c_str());
}
