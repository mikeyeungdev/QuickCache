cmake_minimum_required(VERSION 3.16)

project(QuickCache VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(QUICKCACHE_BUILD_TESTS "Build QuickCache tests" ON)

add_library(quickcache_lib
    src/cache.cpp
    src/parser.cpp
    src/persistence.cpp
    src/server.cpp
    src/stats.cpp
)

target_include_directories(quickcache_lib PUBLIC include)

if(WIN32)
    target_link_libraries(quickcache_lib PUBLIC ws2_32)
endif()

add_executable(quickcache src/main.cpp)
target_link_libraries(quickcache PRIVATE quickcache_lib)

if(QUICKCACHE_BUILD_TESTS)
    include(CTest)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(quickcache_tests
        tests/cache_test.cpp
        tests/parser_test.cpp
        tests/persistence_test.cpp
    )
    target_link_libraries(quickcache_tests PRIVATE quickcache_lib GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(quickcache_tests)
endif()
