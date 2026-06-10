#pragma once

#include <chrono>
#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "stats.h"

namespace quickcache {

class Cache {
public:
    explicit Cache(std::size_t max_keys);

    void set(const std::string& key, const std::string& value, std::optional<int> ttl_seconds = std::nullopt);
    std::optional<std::string> get(const std::string& key);
    bool erase(const std::string& key);
    bool expire(const std::string& key, int ttl_seconds);
    int ttl(const std::string& key);
    std::size_t size();
    StatsSnapshot stats();

private:
    using Clock = std::chrono::steady_clock;
    using Expiry = std::optional<Clock::time_point>;

    struct Entry {
        std::string value;
        Expiry expires_at;
        std::list<std::string>::iterator lru_it;
    };

    bool is_expired(const Entry& entry, Clock::time_point now) const;
    void touch(std::unordered_map<std::string, Entry>::iterator it);
    void erase_it(std::unordered_map<std::string, Entry>::iterator it);
    void evict_if_needed();

    std::size_t max_keys_;
    std::unordered_map<std::string, Entry> entries_;
    std::list<std::string> lru_;
    Stats stats_;
    mutable std::mutex mutex_;
};

} // namespace quickcache
