#include "cache.h"

#include <cassert>
#include <iostream>

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

} // namespace

int main() {
    test_set_and_get();
    test_get_missing_key();
    test_overwrite_value();
    test_remove_existing_key();
    test_remove_missing_key();
    test_empty_key_is_error();

    std::cout << "cache_tests passed" << std::endl;
    return 0;
}
