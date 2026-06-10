# QuickCache

QuickCache is a Redis-style in-memory cache written in C++17. It is designed for short-lived backend data such as verification codes, ticket queue positions, and inventory holds.

## Features

- Thread-safe in-memory key-value store
- TCP command server with concurrent client handling
- TTL expiration with `EXPIRE` and `SET ... EX`
- LRU eviction when the configured key limit is reached
- Append-only persistence log
- Crash recovery by replaying persisted writes with absolute TTL deadlines
- Basic runtime stats
- Google Test coverage for cache, parser, and recovery behavior
- Python benchmark client with mixed high-traffic workloads

## Commands

Commands are newline-delimited text:

```text
PING
SET key value
SET key value EX seconds
GET key
DELETE key
EXPIRE key seconds
TTL key
STATS
```

Responses are simple text values:

- `PONG` for `PING`
- `OK` for successful writes
- `1` or `0` for `DELETE` and `EXPIRE`
- stored value or `(nil)` for `GET`
- seconds, `-1`, or `-2` for `TTL`
- stats as `key=value` pairs

## Build

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

If you only want the server and want to skip Google Test fetching:

```powershell
cmake -S . -B build -DQUICKCACHE_BUILD_TESTS=OFF
cmake --build build
```

## Run

```powershell
.\build\Debug\quickcache.exe --port 6379 --max-keys 100000 --log quickcache.aof
```

Linux/macOS:

```bash
./build/quickcache --port 6379 --max-keys 100000 --log quickcache.aof
```

Then connect with `nc`:

```bash
nc 127.0.0.1 6379
SET code:user123 849201 EX 300
GET code:user123
TTL code:user123
```

## Demo Use Cases

```text
SET code:user123 849201 EX 300
SET queue:user123 482 EX 1800
SET hold:item456:user123 reserved EX 600
```

## Benchmark

Start the server, then run:

```powershell
python .\benchmarks\benchmark_client.py --clients 64 --requests 50000 --workload .\benchmarks\workloads\verification_codes.txt
```

The benchmark reports requests per second, p50/p95/p99 latency, error count, and a server `STATS` sample.

## Persistence Notes

The public TCP command format uses relative TTLs (`EX seconds`). The append-only log stores TTL-bearing writes with internal absolute expiration deadlines so restart recovery does not refresh already-expired keys.
