cmake_minimum_required(VERSION 3.16)

project(QuickCache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(quickcache_core
    src/cache.cpp
)

target_include_directories(quickcache_core PUBLIC include)

add_executable(quickcache src/main.cpp)
target_link_libraries(quickcache PRIVATE quickcache_core)

enable_testing()

add_executable(cache_tests tests/cache_test.cpp)
target_link_libraries(cache_tests PRIVATE quickcache_core)

add_test(NAME cache_tests COMMAND cache_tests)
