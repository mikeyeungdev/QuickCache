#include "cache.h"
#include "persistence.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

namespace {

std::string testPath(const std::string& name) {
    return name + ".aof";
}

void removeFile(const std::string& path) {
    std::remove(path.c_str());
}

void test_set_persists_after_restart() {
    const auto path = testPath("quickcache_set_test");
    removeFile(path);

    {
        quickcache::AppendOnlyLog log(path);
        assert(log.openForAppend());
        log.appendSet("code:user123", "849201");
    }

    quickcache::Cache recovered;
    quickcache::AppendOnlyLog log(path);
    assert(log.replay(recovered));
    assert(recovered.get("code:user123").value() == "849201");

    removeFile(path);
}

void test_delete_persists_after_restart() {
    const auto path = testPath("quickcache_delete_test");
    removeFile(path);

    {
        quickcache::AppendOnlyLog log(path);
        assert(log.openForAppend());
        log.appendSet("code:user123", "849201");
        log.appendDelete("code:user123");
    }

    quickcache::Cache recovered;
    quickcache::AppendOnlyLog log(path);
    assert(log.replay(recovered));
    assert(!recovered.get("code:user123").has_value());

    removeFile(path);
}

void test_expire_persists_after_restart() {
    const auto path = testPath("quickcache_expire_test");
    removeFile(path);

    {
        quickcache::AppendOnlyLog log(path);
        assert(log.openForAppend());
        log.appendSet("queue:user123", "482");
        log.appendExpire("queue:user123", 30);
    }

    quickcache::Cache recovered;
    quickcache::AppendOnlyLog log(path);
    assert(log.replay(recovered));
    assert(recovered.get("queue:user123").value() == "482");
    assert(recovered.ttl("queue:user123").has_value());

    removeFile(path);
}

void test_expired_keys_do_not_return_after_restart() {
    const auto path = testPath("quickcache_expired_test");
    removeFile(path);

    {
        quickcache::AppendOnlyLog log(path);
        assert(log.openForAppend());
        log.appendSet("code:user123", "849201", 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    quickcache::Cache recovered;
    quickcache::AppendOnlyLog log(path);
    assert(log.replay(recovered));
    assert(!recovered.get("code:user123").has_value());

    removeFile(path);
}

} // namespace

int main() {
    test_set_persists_after_restart();
    test_delete_persists_after_restart();
    test_expire_persists_after_restart();
    test_expired_keys_do_not_return_after_restart();

    std::cout << "persistence_tests passed" << std::endl;
    return 0;
}
