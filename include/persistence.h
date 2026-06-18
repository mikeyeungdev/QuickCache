#pragma once

#include <fstream>
#include <mutex>
#include <optional>
#include <string>

namespace quickcache {

class Cache;

class AppendOnlyLog {
public:
    explicit AppendOnlyLog(std::string path);

    bool replay(Cache& cache) const;
    bool openForAppend();

    void appendSet(const std::string& key, const std::string& value);
    void appendSet(const std::string& key, const std::string& value, int ttl_seconds);
    void appendDelete(const std::string& key);
    void appendExpire(const std::string& key, int ttl_seconds);

    const std::string& path() const;

private:
    static std::string encode(const std::string& value);
    static std::string decode(const std::string& value);
    static long long currentEpochSeconds();

    void appendLine(const std::string& line);

    std::string path_;
    std::ofstream output_;
    std::mutex mutex_;
};

} // namespace quickcache
