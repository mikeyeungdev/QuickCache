#pragma once

#include <atomic>
#include <cstdint>

namespace quickcache {

struct StatsSnapshot {
    std::uint64_t hits = 0;
    std::uint64_t misses = 0;
    std::uint64_t sets = 0;
    std::uint64_t deletes = 0;
    std::uint64_t expirations = 0;
    std::uint64_t evictions = 0;
    std::uint64_t commands = 0;
    std::uint64_t keys = 0;
};

class Stats {
public:
    void hit();
    void miss();
    void set();
    void erase();
    void expiration();
    void eviction();
    void command();
    StatsSnapshot snapshot(std::uint64_t keys) const;

private:
    std::atomic<std::uint64_t> hits_{0};
    std::atomic<std::uint64_t> misses_{0};
    std::atomic<std::uint64_t> sets_{0};
    std::atomic<std::uint64_t> deletes_{0};
    std::atomic<std::uint64_t> expirations_{0};
    std::atomic<std::uint64_t> evictions_{0};
    std::atomic<std::uint64_t> commands_{0};
};

} // namespace quickcache
