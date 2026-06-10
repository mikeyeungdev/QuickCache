#pragma once

#include <fstream>
#include <mutex>
#include <optional>
#include <string>

namespace quickcache {

class Cache;

class PersistenceLog {
public:
    explicit PersistenceLog(std::string path);

    bool open();
    void append_set(const std::string& key, const std::string& value, std::optional<int> ttl_seconds);
    void append_delete(const std::string& key);
    void append_expire(const std::string& key, int ttl_seconds);
    void replay(Cache& cache);
    const std::string& path() const;

private:
    static std::string encode(const std::string& value);
    static std::string decode(const std::string& value);

    std::string path_;
    std::ofstream stream_;
    std::mutex mutex_;
};

} // namespace quickcache
