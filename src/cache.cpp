#include "cache.h"

#include <algorithm>
#include <cstdint>

namespace quickcache {

Cache::Cache(std::size_t max_keys) : max_keys_(std::max<std::size_t>(1, max_keys)) {}

void Cache::set(const std::string& key, const std::string& value, std::optional<int> ttl_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.command();
    const auto now = Clock::now();
    Expiry expires_at = std::nullopt;
    if (ttl_seconds.has_value()) {
        expires_at = now + std::chrono::seconds(*ttl_seconds);
    }

    auto it = entries_.find(key);
    if (it != entries_.end()) {
        it->second.value = value;
        it->second.expires_at = expires_at;
        touch(it);
    } else {
        lru_.push_front(key);
        entries_.emplace(key, Entry{value, expires_at, lru_.begin()});
    }

    stats_.set();
    evict_if_needed();
}

std::optional<std::string> Cache::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.command();
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        stats_.miss();
        return std::nullopt;
    }

    if (is_expired(it->second, Clock::now())) {
        erase_it(it);
        stats_.expiration();
        stats_.miss();
        return std::nullopt;
    }

    touch(it);
    stats_.hit();
    return it->second.value;
}

bool Cache::erase(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.command();
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return false;
    }
    erase_it(it);
    stats_.erase();
    return true;
}

bool Cache::expire(const std::string& key, int ttl_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.command();
    auto it = entries_.find(key);
    if (it == entries_.end() || is_expired(it->second, Clock::now())) {
        if (it != entries_.end()) {
            erase_it(it);
            stats_.expiration();
        }
        return false;
    }

    it->second.expires_at = Clock::now() + std::chrono::seconds(ttl_seconds);
    touch(it);
    return true;
}

int Cache::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.command();
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return -2;
    }

    const auto now = Clock::now();
    if (is_expired(it->second, now)) {
        erase_it(it);
        stats_.expiration();
        return -2;
    }

    if (!it->second.expires_at.has_value()) {
        return -1;
    }

    const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(*it->second.expires_at - now);
    return static_cast<int>(std::max<std::int64_t>(0, remaining.count()));
}

std::size_t Cache::size() {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

StatsSnapshot Cache::stats() {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_.snapshot(entries_.size());
}

bool Cache::is_expired(const Entry& entry, Clock::time_point now) const {
    return entry.expires_at.has_value() && now >= *entry.expires_at;
}

void Cache::touch(std::unordered_map<std::string, Entry>::iterator it) {
    lru_.erase(it->second.lru_it);
    lru_.push_front(it->first);
    it->second.lru_it = lru_.begin();
}

void Cache::erase_it(std::unordered_map<std::string, Entry>::iterator it) {
    lru_.erase(it->second.lru_it);
    entries_.erase(it);
}

void Cache::evict_if_needed() {
    while (entries_.size() > max_keys_) {
        const auto key = lru_.back();
        auto it = entries_.find(key);
        if (it != entries_.end()) {
            erase_it(it);
            stats_.eviction();
        }
    }
}

} // namespace quickcache
