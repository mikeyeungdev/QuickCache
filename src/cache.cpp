#include "cache.h"

namespace quickcache {

CacheStatus Cache::set(const std::string& key, const std::string& value) {
    if (key.empty()) {
        return CacheStatus::Error;
    }

    entries_[key] = Entry{value, std::nullopt};
    return CacheStatus::Ok;
}

CacheStatus Cache::set(const std::string& key, const std::string& value, int ttl_seconds) {
    if (key.empty() || !isValidTtl(ttl_seconds)) {
        return CacheStatus::Error;
    }

    entries_[key] = Entry{value, expirationFromNow(ttl_seconds)};
    return CacheStatus::Ok;
}

std::optional<std::string> Cache::get(const std::string& key) {
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return std::nullopt;
    }

    if (isExpired(it->second, Clock::now())) {
        entries_.erase(it);
        return std::nullopt;
    }

    return it->second.value;
}

CacheStatus Cache::remove(const std::string& key) {
    const auto removed = entries_.erase(key);
    return removed == 0 ? CacheStatus::NotFound : CacheStatus::Ok;
}

CacheStatus Cache::expire(const std::string& key, int ttl_seconds) {
    if (!isValidTtl(ttl_seconds)) {
        return CacheStatus::Error;
    }

    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return CacheStatus::NotFound;
    }

    if (isExpired(it->second, Clock::now())) {
        entries_.erase(it);
        return CacheStatus::NotFound;
    }

    it->second.expires_at = expirationFromNow(ttl_seconds);
    return CacheStatus::Ok;
}

std::optional<int> Cache::ttl(const std::string& key) {
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return std::nullopt;
    }

    const auto now = Clock::now();
    if (isExpired(it->second, now)) {
        entries_.erase(it);
        return std::nullopt;
    }

    if (!it->second.expires_at.has_value()) {
        return std::nullopt;
    }

    const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(*it->second.expires_at - now);
    return static_cast<int>(remaining.count());
}

std::size_t Cache::cleanupExpired() {
    const auto now = Clock::now();
    std::size_t removed = 0;

    for (auto it = entries_.begin(); it != entries_.end();) {
        if (isExpired(it->second, now)) {
            it = entries_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }

    return removed;
}

std::size_t Cache::size() const {
    return entries_.size();
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

} // namespace quickcache
