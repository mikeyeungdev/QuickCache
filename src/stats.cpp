#include "stats.h"

namespace quickcache {

void Stats::hit() {
    hits_.fetch_add(1, std::memory_order_relaxed);
}

void Stats::miss() {
    misses_.fetch_add(1, std::memory_order_relaxed);
}

void Stats::set() {
    sets_.fetch_add(1, std::memory_order_relaxed);
}

void Stats::erase() {
    deletes_.fetch_add(1, std::memory_order_relaxed);
}

void Stats::expiration() {
    expirations_.fetch_add(1, std::memory_order_relaxed);
}

void Stats::eviction() {
    evictions_.fetch_add(1, std::memory_order_relaxed);
}

void Stats::command() {
    commands_.fetch_add(1, std::memory_order_relaxed);
}

StatsSnapshot Stats::snapshot(std::uint64_t keys) const {
    return {
        hits_.load(std::memory_order_relaxed),
        misses_.load(std::memory_order_relaxed),
        sets_.load(std::memory_order_relaxed),
        deletes_.load(std::memory_order_relaxed),
        expirations_.load(std::memory_order_relaxed),
        evictions_.load(std::memory_order_relaxed),
        commands_.load(std::memory_order_relaxed),
        keys,
    };
}

} // namespace quickcache
