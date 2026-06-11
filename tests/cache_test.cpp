#include "cache.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

namespace {

void test_set_and_get() {
    quickcache::Cache cache;

    assert(cache.set("code:user123", "849201") == quickcache::CacheStatus::Ok);
    assert(cache.get("code:user123").has_value());
    assert(cache.get("code:user123").value() == "849201");
    assert(cache.size() == 1);
}

void test_get_missing_key() {
    quickcache::Cache cache;

    assert(!cache.get("code:user123").has_value());
}

void test_overwrite_value() {
    quickcache::Cache cache;

    assert(cache.set("code:user123", "111111") == quickcache::CacheStatus::Ok);
    assert(cache.set("code:user123", "849201") == quickcache::CacheStatus::Ok);
    assert(cache.get("code:user123").value() == "849201");
    assert(cache.size() == 1);
}

void test_remove_existing_key() {
    quickcache::Cache cache;

    assert(cache.set("code:user123", "849201") == quickcache::CacheStatus::Ok);
    assert(cache.remove("code:user123") == quickcache::CacheStatus::Ok);
    assert(!cache.get("code:user123").has_value());
    assert(cache.size() == 0);
}

void test_remove_missing_key() {
    quickcache::Cache cache;

    assert(cache.remove("code:user123") == quickcache::CacheStatus::NotFound);
}

void test_empty_key_is_error() {
    quickcache::Cache cache;

    assert(cache.set("", "849201") == quickcache::CacheStatus::Error);
    assert(cache.size() == 0);
}

void test_key_exists_before_ttl_expires() {
    quickcache::Cache cache;

    assert(cache.set("code:user123", "849201", 1) == quickcache::CacheStatus::Ok);
    assert(cache.get("code:user123").value() == "849201");
}

void test_key_is_gone_after_ttl_expires() {
    quickcache::Cache cache;

    assert(cache.set("code:user123", "849201", 1) == quickcache::CacheStatus::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    assert(!cache.get("code:user123").has_value());
}

void test_expire_updates_ttl() {
    quickcache::Cache cache;

    assert(cache.set("queue:user123", "482") == quickcache::CacheStatus::Ok);
    assert(cache.expire("queue:user123", 2) == quickcache::CacheStatus::Ok);
    assert(cache.ttl("queue:user123").has_value());
    assert(cache.ttl("queue:user123").value() <= 2);
    assert(cache.ttl("queue:user123").value() >= 0);
}

void test_ttl_returns_remaining_time() {
    quickcache::Cache cache;

    assert(cache.set("hold:item456:user123", "reserved", 2) == quickcache::CacheStatus::Ok);
    const auto remaining = cache.ttl("hold:item456:user123");

    assert(remaining.has_value());
    assert(remaining.value() <= 2);
    assert(remaining.value() >= 0);
}

void test_expired_key_is_removed_on_get() {
    quickcache::Cache cache;

    assert(cache.set("code:user123", "849201", 0) == quickcache::CacheStatus::Ok);
    assert(cache.size() == 1);
    assert(!cache.get("code:user123").has_value());
    assert(cache.size() == 0);
}

void test_cleanup_expired_removes_expired_keys() {
    quickcache::Cache cache;

    assert(cache.set("expired:key", "old", 0) == quickcache::CacheStatus::Ok);
    assert(cache.set("active:key", "new") == quickcache::CacheStatus::Ok);

    assert(cache.cleanupExpired() == 1);
    assert(cache.size() == 1);
    assert(cache.get("active:key").value() == "new");
}

} // namespace

int main() {
    test_set_and_get();
    test_get_missing_key();
    test_overwrite_value();
    test_remove_existing_key();
    test_remove_missing_key();
    test_empty_key_is_error();
    test_key_exists_before_ttl_expires();
    test_key_is_gone_after_ttl_expires();
    test_expire_updates_ttl();
    test_ttl_returns_remaining_time();
    test_expired_key_is_removed_on_get();
    test_cleanup_expired_removes_expired_keys();

    std::cout << "cache_tests passed" << std::endl;
    return 0;
}
