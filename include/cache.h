#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace quickcache {

enum class CacheStatus {
    Ok,
    NotFound,
    Error
};

class Cache {
public:
    CacheStatus set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    CacheStatus remove(const std::string& key);
    std::size_t size() const;

private:
    std::unordered_map<std::string, std::string> entries_;
};

} // namespace quickcache
