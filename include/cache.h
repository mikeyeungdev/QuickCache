#pragma once

#include <chrono>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "stats.h"

namespace quickcache {

enum class CacheStatus {
    Ok,
    NotFound,
    Error
};

class Cache {
public:
    explicit Cache(std::size_t max_keys = 1000, RuntimeStats* stats = nullptr);

    CacheStatus set(const std::string& key, const std::string& value);
    CacheStatus set(const std::string& key, const std::string& value, int ttl_seconds);
    std::optional<std::string> get(const std::string& key);
    CacheStatus remove(const std::string& key);
    CacheStatus expire(const std::string& key, int ttl_seconds);
    std::optional<int> ttl(const std::string& key);
    std::size_t cleanupExpired();
    std::size_t size() const;
    std::size_t approximateMemoryUsage() const;

private:
    using Clock = std::chrono::steady_clock;

    struct Entry {
        std::string value;
        std::optional<Clock::time_point> expires_at;
        std::list<std::string>::iterator lru_position;
    };

    static bool isValidTtl(int ttl_seconds);
    bool isExpired(const Entry& entry, Clock::time_point now) const;
    std::optional<Clock::time_point> expirationFromNow(int ttl_seconds) const;
    void touch(std::unordered_map<std::string, Entry>::iterator entry);
    void eraseEntry(std::unordered_map<std::string, Entry>::iterator entry);
    void evictIfNeeded();
    std::size_t cleanupExpiredUnlocked();

    std::size_t max_keys_;
    std::list<std::string> lru_order_;
    std::unordered_map<std::string, Entry> entries_;
    RuntimeStats* stats_;
    mutable std::mutex mutex_;
};

} // namespace quickcache
