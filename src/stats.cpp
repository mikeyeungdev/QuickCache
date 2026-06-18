#include "stats.h"

namespace quickcache {

RuntimeStats::RuntimeStats() : started_at_(Clock::now()) {}

void RuntimeStats::recordCommand() {
    total_commands_.fetch_add(1, std::memory_order_relaxed);
}

void RuntimeStats::recordGetHit() {
    get_hits_.fetch_add(1, std::memory_order_relaxed);
}

void RuntimeStats::recordGetMiss() {
    get_misses_.fetch_add(1, std::memory_order_relaxed);
}

void RuntimeStats::recordExpiredKeysRemoved(std::uint64_t count) {
    expired_keys_removed_.fetch_add(count, std::memory_order_relaxed);
}

void RuntimeStats::recordEvictedKey() {
    evicted_keys_.fetch_add(1, std::memory_order_relaxed);
}

StatsSnapshot RuntimeStats::snapshot(std::size_t total_keys, std::size_t approximate_memory_bytes) const {
    const auto uptime = std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - started_at_);

    StatsSnapshot snapshot;
    snapshot.total_commands = total_commands_.load(std::memory_order_relaxed);
    snapshot.get_hits = get_hits_.load(std::memory_order_relaxed);
    snapshot.get_misses = get_misses_.load(std::memory_order_relaxed);
    snapshot.expired_keys_removed = expired_keys_removed_.load(std::memory_order_relaxed);
    snapshot.evicted_keys = evicted_keys_.load(std::memory_order_relaxed);
    snapshot.uptime_seconds = static_cast<std::uint64_t>(uptime.count());
    snapshot.total_keys = total_keys;
    snapshot.approximate_memory_bytes = approximate_memory_bytes;
    return snapshot;
}

} // namespace quickcache
