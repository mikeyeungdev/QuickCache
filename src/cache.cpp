#include "cache.h"

namespace quickcache {

CacheStatus Cache::set(const std::string& key, const std::string& value) {
    if (key.empty()) {
        return CacheStatus::Error;
    }

    entries_[key] = value;
    return CacheStatus::Ok;
}

std::optional<std::string> Cache::get(const std::string& key) const {
    const auto it = entries_.find(key);
    if (it == entries_.end()) {
        return std::nullopt;
    }

    return it->second;
}

CacheStatus Cache::remove(const std::string& key) {
    const auto removed = entries_.erase(key);
    return removed == 0 ? CacheStatus::NotFound : CacheStatus::Ok;
}

std::size_t Cache::size() const {
    return entries_.size();
}

} // namespace quickcache
