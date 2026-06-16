cmake_minimum_required(VERSION 3.16)

project(QuickCache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/EHsc)
endif()

add_library(quickcache_core
    src/cache.cpp
    src/parser.cpp
    src/server.cpp
)

target_include_directories(quickcache_core PUBLIC include)

if(WIN32)
    target_link_libraries(quickcache_core PUBLIC ws2_32)
endif()

add_executable(quickcache src/main.cpp)
target_link_libraries(quickcache PRIVATE quickcache_core)

enable_testing()

add_executable(cache_tests tests/cache_test.cpp)
target_link_libraries(cache_tests PRIVATE quickcache_core)

add_test(NAME cache_tests COMMAND cache_tests)

add_executable(parser_tests tests/parser_test.cpp)
target_link_libraries(parser_tests PRIVATE quickcache_core)

add_test(NAME parser_tests COMMAND parser_tests)
