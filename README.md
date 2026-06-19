# QuickCache

Backend systems often need fast access to short-lived data such as verification codes, ticket queue positions, and inventory holds. Storing that data in a primary database can add unnecessary latency and cleanup work, especially when the data only needs to live for seconds or minutes.

QuickCache is a Redis-style in-memory cache written in C++. It exposes a newline-delimited TCP protocol, stores string keys and values, supports TTL expiration, evicts least recently used keys when full, and can rebuild cache state from an append-only log after restart.

## Demo Use Cases

- **Ticket queue positions**: keep a user's current place in a high-traffic queue.
  `SET queue:user123 482 EX 1800`
- **Verification codes**: store short-lived login or account verification codes.
  `SET code:user123 849201 EX 300`
- **Inventory holds**: reserve an item briefly while a user checks out.
  `SET hold:item456:user123 reserved EX 600`

## Architecture

```text
Client
  |
  v
TCP Server
  |
  v
Command Parser
  |
  v
Cache Engine
  |
  +--> TTL Expiration
  +--> LRU Eviction
  +--> Append-Only Persistence
```

## Features

- TCP server with one thread per connected client
- Thread-safe cache operations using `std::mutex`
- String key/value storage
- TTL expiration with lazy cleanup
- LRU eviction with a configurable max key limit
- Append-only persistence and crash recovery
- Runtime `STATS` metrics
- Python and C++ benchmark clients for realistic workloads

## Commands

Commands are newline-delimited text.

```text
PING
PONG

SET code:user123 849201 EX 300
OK

GET code:user123
VALUE 849201

TTL code:user123
TTL 299

DELETE code:user123
OK

GET code:user123
NOT_FOUND

STATS
OK total_keys=1 expired_keys_removed=0 evicted_keys=0 total_commands=7 get_hits=1 get_misses=1 uptime_seconds=12 approximate_memory_bytes=512
```

Supported syntax:

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

Malformed commands return `ERROR <message>` instead of crashing.

## Build

From the repository root:

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

On Windows with Visual Studio generators, the executables are usually under `Debug/` or `Release/`.

## Run

Linux/macOS:

```bash
./quickcache --port 6379 --max-keys 1000 --aof quickcache.aof
```

Windows:

```powershell
.\Debug\quickcache.exe --port 6379 --max-keys 1000 --aof quickcache.aof
```

If `--aof` is omitted, QuickCache uses `quickcache.aof` by default. To start fresh, stop the server and delete the AOF file or use a new filename.

## Test

From the build directory:

```bash
ctest --output-on-failure
```

On Windows with Visual Studio generators:

```powershell
ctest -C Debug --output-on-failure
```

## TCP Demo

Start the server, then connect with netcat or Ncat:

```bash
nc localhost 6379
```

Windows with Ncat:

```powershell
& "C:\Program Files (x86)\Nmap\ncat.exe" localhost 6379
```

Try:

```text
PING
SET code:user123 849201 EX 300
GET code:user123
STATS
DELETE code:user123
```

## Append-Only Persistence

Successful write commands are appended to the AOF:

```text
SET key value
SET key value EX seconds
DELETE key
EXPIRE key seconds
```

On startup, QuickCache replays the log before accepting clients. Replay does not re-append commands, so recovery does not duplicate the log.

TTL-bearing writes are stored internally with absolute expiration timestamps, such as `EXAT <epoch_seconds>` and `EXPIREAT <epoch_seconds>`. This prevents a key from receiving a fresh TTL after restart. If a verification code expires while the server is offline, replay skips it.

## Benchmarks

Start QuickCache:

```bash
./quickcache --port 6379 --max-keys 100000 --aof benchmark.aof
```

Run a workload:

```bash
python3 benchmarks/benchmark_client.py --host localhost --port 6379 --clients 50 --requests 10000 --workload verification_codes
```

Windows:

```powershell
python .\benchmarks\benchmark_client.py --host localhost --port 6379 --clients 50 --requests 10000 --workload verification_codes
```

For higher-throughput Release benchmarks, build the C++ benchmark target and run:

```powershell
.\Release\benchmark_client.exe --host 127.0.0.1 --port 6379 --clients 200 --requests 200000 --pipeline 1 --workload verification_codes
```

Workloads:

- `verification_codes`: alternating `SET code:userId value EX 300` and `GET code:userId`
- `ticket_queue`: repeated `SET queue:userId position EX 1800` and `GET queue:userId`
- `inventory_holds`: `SET hold:itemId:userId reserved EX 600`, `GET`, and `DELETE`

The benchmark reports total requests, successful requests, errors, requests per second, and p50/p95/p99 latency.

## Benchmark Results

Sample local Release result on a Windows development machine, using the C++ benchmark client with AOF persistence enabled:

```text
workload: verification_codes
clients: 200
pipeline: 1
total_requests: 200000
successful_requests: 200000
errors: 0
requests_per_second: 123779
latency_p50_ms: 0.079
latency_p95_ms: 6.183
latency_p99_ms: 9.743
```

These are local benchmark numbers, not a formal production benchmark. Results depend on machine, build mode, client implementation, and OS networking behavior.

## Design Tradeoffs

- **In-memory speed vs durability**: reads and writes are fast because data lives in memory, but durability depends on replaying the append-only log after restart.
- **Append-only recovery vs full database storage**: AOF replay is simple and transparent, but the log can grow over time and does not replace a full database for permanent records.
- **Simple thread model vs production-grade event loop**: one thread per client is easy to reason about and demonstrates concurrency, but a production cache would likely use an event loop, async I/O, or a bounded worker pool.

## Future Improvements

- Authentication
- Replication
- Clustering
- Snapshots and snapshot compaction
- Richer data types
- Bounded thread pool or event-driven networking
