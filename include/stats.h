#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace quickcache {

struct StatsSnapshot {
    std::uint64_t total_commands = 0;
    std::uint64_t get_hits = 0;
    std::uint64_t get_misses = 0;
    std::uint64_t expired_keys_removed = 0;
    std::uint64_t evicted_keys = 0;
    std::uint64_t uptime_seconds = 0;
    std::size_t total_keys = 0;
    std::size_t approximate_memory_bytes = 0;
};

class RuntimeStats {
public:
    RuntimeStats();

    void recordCommand();
    void recordGetHit();
    void recordGetMiss();
    void recordExpiredKeysRemoved(std::uint64_t count);
    void recordEvictedKey();

    StatsSnapshot snapshot(std::size_t total_keys, std::size_t approximate_memory_bytes) const;

private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point started_at_;
    std::atomic<std::uint64_t> total_commands_{0};
    std::atomic<std::uint64_t> get_hits_{0};
    std::atomic<std::uint64_t> get_misses_{0};
    std::atomic<std::uint64_t> expired_keys_removed_{0};
    std::atomic<std::uint64_t> evicted_keys_{0};
};

} // namespace quickcache
