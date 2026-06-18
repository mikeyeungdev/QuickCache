#include "cache.h"
#include "stats.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

namespace {

void test_runtime_stats_counts_commands_and_uptime() {
    quickcache::RuntimeStats stats;

    stats.recordCommand();
    stats.recordCommand();
    stats.recordGetHit();
    stats.recordGetMiss();

    const auto snapshot = stats.snapshot(3, 512);

    assert(snapshot.total_commands == 2);
    assert(snapshot.get_hits == 1);
    assert(snapshot.get_misses == 1);
    assert(snapshot.total_keys == 3);
    assert(snapshot.approximate_memory_bytes == 512);
    assert(snapshot.uptime_seconds >= 0);
}

void test_cache_updates_hit_miss_expired_and_eviction_stats() {
    quickcache::RuntimeStats stats;
    quickcache::Cache cache(1, &stats);

    assert(cache.set("hit:key", "value") == quickcache::CacheStatus::Ok);
    assert(cache.get("hit:key").value() == "value");
    assert(!cache.get("missing:key").has_value());

    assert(cache.set("expired:key", "old", 0) == quickcache::CacheStatus::Ok);
    assert(!cache.get("expired:key").has_value());

    assert(cache.set("a", "1") == quickcache::CacheStatus::Ok);
    assert(cache.set("b", "2") == quickcache::CacheStatus::Ok);

    const auto snapshot = stats.snapshot(cache.size(), cache.approximateMemoryUsage());

    assert(snapshot.get_hits == 1);
    assert(snapshot.get_misses == 2);
    assert(snapshot.expired_keys_removed >= 1);
    assert(snapshot.evicted_keys >= 1);
    assert(snapshot.total_keys == 1);
    assert(snapshot.approximate_memory_bytes > 0);
}

} // namespace

int main() {
    test_runtime_stats_counts_commands_and_uptime();
    test_cache_updates_hit_miss_expired_and_eviction_stats();

    std::cout << "stats_tests passed" << std::endl;
    return 0;
}
