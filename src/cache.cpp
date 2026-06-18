#include "cache.h"

#include <algorithm>
#include <mutex>

namespace quickcache {

Cache::Cache(std::size_t max_keys, RuntimeStats* stats)
    : max_keys_(std::max<std::size_t>(1, max_keys)), stats_(stats) {}

CacheStatus Cache::set(const std::string& key, const std::string& value) {
    if (key.empty()) {
        return CacheStatus::Error;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it != entries_.end()) {
        it->second.value = value;
        it->second.expires_at = std::nullopt;
        touch(it);
    } else {
        if (entries_.size() >= max_keys_) {
            cleanupExpiredUnlocked();
        }
        lru_order_.push_front(key);
        entries_.emplace(key, Entry{value, std::nullopt, lru_order_.begin()});
        evictIfNeeded();
    }

    return CacheStatus::Ok;
}

CacheStatus Cache::set(const std::string& key, const std::string& value, int ttl_seconds) {
    if (key.empty() || !isValidTtl(ttl_seconds)) {
        return CacheStatus::Error;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it != entries_.end()) {
        it->second.value = value;
        it->second.expires_at = expirationFromNow(ttl_seconds);
        touch(it);
    } else {
        if (entries_.size() >= max_keys_) {
            cleanupExpiredUnlocked();
        }
        lru_order_.push_front(key);
        entries_.emplace(key, Entry{value, expirationFromNow(ttl_seconds), lru_order_.begin()});
        evictIfNeeded();
    }

    return CacheStatus::Ok;
}

std::optional<std::string> Cache::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it == entries_.end()) {
        if (stats_ != nullptr) {
            stats_->recordGetMiss();
        }
        return std::nullopt;
    }

    if (isExpired(it->second, Clock::now())) {
        eraseEntry(it);
        if (stats_ != nullptr) {
            stats_->recordExpiredKeysRemoved(1);
            stats_->recordGetMiss();
        }
        return std::nullopt;
    }

    touch(it);
    if (stats_ != nullptr) {
        stats_->recordGetHit();
    }
    return it->second.value;
}

CacheStatus Cache::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return CacheStatus::NotFound;
    }

    eraseEntry(it);
    return CacheStatus::Ok;
}

CacheStatus Cache::expire(const std::string& key, int ttl_seconds) {
    if (!isValidTtl(ttl_seconds)) {
        return CacheStatus::Error;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return CacheStatus::NotFound;
    }

    if (isExpired(it->second, Clock::now())) {
        eraseEntry(it);
        if (stats_ != nullptr) {
            stats_->recordExpiredKeysRemoved(1);
        }
        return CacheStatus::NotFound;
    }

    it->second.expires_at = expirationFromNow(ttl_seconds);
    touch(it);
    return CacheStatus::Ok;
}

std::optional<int> Cache::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return std::nullopt;
    }

    const auto now = Clock::now();
    if (isExpired(it->second, now)) {
        eraseEntry(it);
        if (stats_ != nullptr) {
            stats_->recordExpiredKeysRemoved(1);
        }
        return std::nullopt;
    }

    if (!it->second.expires_at.has_value()) {
        return std::nullopt;
    }

    const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(*it->second.expires_at - now);
    return static_cast<int>(remaining.count());
}

std::size_t Cache::cleanupExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    return cleanupExpiredUnlocked();
}

std::size_t Cache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

std::size_t Cache::approximateMemoryUsage() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::size_t total = sizeof(*this);
    total += entries_.bucket_count() * sizeof(void*);
    total += lru_order_.size() * (sizeof(std::string) + (sizeof(void*) * 2));

    for (const auto& item : entries_) {
        total += sizeof(item);
        total += item.first.capacity();
        total += item.second.value.capacity();
    }

    return total;
}

bool Cache::isValidTtl(int ttl_seconds) {
    return ttl_seconds >= 0;
}

bool Cache::isExpired(const Entry& entry, Clock::time_point now) const {
    return entry.expires_at.has_value() && now >= *entry.expires_at;
}

std::optional<Cache::Clock::time_point> Cache::expirationFromNow(int ttl_seconds) const {
    return Clock::now() + std::chrono::seconds(ttl_seconds);
}

void Cache::touch(std::unordered_map<std::string, Entry>::iterator entry) {
    lru_order_.erase(entry->second.lru_position);
    lru_order_.push_front(entry->first);
    entry->second.lru_position = lru_order_.begin();
}

void Cache::eraseEntry(std::unordered_map<std::string, Entry>::iterator entry) {
    lru_order_.erase(entry->second.lru_position);
    entries_.erase(entry);
}

void Cache::evictIfNeeded() {
    while (entries_.size() > max_keys_) {
        const auto key_to_evict = lru_order_.back();
        auto it = entries_.find(key_to_evict);
        if (it != entries_.end()) {
            eraseEntry(it);
            if (stats_ != nullptr) {
                stats_->recordEvictedKey();
            }
        }
    }
}

std::size_t Cache::cleanupExpiredUnlocked() {
    const auto now = Clock::now();
    std::size_t removed = 0;

    for (auto it = entries_.begin(); it != entries_.end();) {
        if (isExpired(it->second, now)) {
            lru_order_.erase(it->second.lru_position);
            it = entries_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }

    if (removed > 0 && stats_ != nullptr) {
        stats_->recordExpiredKeysRemoved(static_cast<std::uint64_t>(removed));
    }

    return removed;
}

} // namespace quickcache
