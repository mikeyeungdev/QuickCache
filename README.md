# QuickCache

QuickCache is a planned Redis-style in-memory cache written in C++. It will provide fast retrieval for short-lived backend data that needs to be available immediately and expire when it is no longer useful.

Short-lived backend data often lives on critical user paths. Ticket queue positions, verification codes, and inventory holds all need low-latency reads and writes because users are waiting on the result in real time. Keeping this data in memory avoids slower database round trips for data that does not need permanent storage.

## Example Use Cases

- Ticket queue positions, such as `queue:user123 -> 482`
- Verification codes, such as `code:user123 -> 849201`
- Inventory holds, such as `hold:item456:user123 -> reserved`

## Current Behavior

The local cache engine currently supports:

- `set(key, value)` for string keys and string values
- `set(key, value, ttl_seconds)` for values that should expire automatically
- `get(key)` returning the stored value or no value when the key is missing
- `remove(key)` returning `Ok` when a key was deleted and `NotFound` when it was absent
- `expire(key, ttl_seconds)` for updating a key's expiration
- `ttl(key)` for reading the remaining lifetime
- `cleanupExpired()` for removing expired keys in a batch
- LRU eviction when the cache reaches its max key limit

Example behavior covered by tests:

```text
SET code:user123 849201
GET code:user123
DELETE code:user123
```

QuickCache now exposes these commands over a newline-delimited TCP protocol.

TTL example:

```text
SET code:user123 849201 EX 3
GET code:user123
# returns 849201

# wait 4 seconds

GET code:user123
# returns NOT_FOUND
```

LRU eviction example:

```text
# max keys = 2
SET a 1
SET b 2
GET a
SET c 3

# a and c remain because a was recently read
# b is evicted as the least recently used key
```

## Planned Commands

- `PING`
- `SET`
- `GET`
- `DELETE`
- `EXPIRE`
- `TTL`
- `STATS`

## Supported Command Syntax

QuickCache now includes a local parser for newline-delimited text commands. The parser turns command text into structured command objects; it does not execute commands or use networking yet.

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

Malformed commands return parse errors instead of crashing. Examples include missing keys, invalid TTL values, unsupported `SET` options, extra arguments, and unknown commands.

## Build

From the repository root:

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Test

From the build directory:

```bash
ctest --output-on-failure
```

On Windows with Visual Studio generators:

```powershell
ctest -C Debug --output-on-failure
```

## Run

From the build directory:

```bash
./quickcache --port 6379 --max-keys 1000 --aof quickcache.aof
```

On Windows with multi-configuration generators, run:

```powershell
.\Debug\quickcache.exe --port 6379 --max-keys 1000 --aof quickcache.aof
```

## TCP Usage

Start the server:

```bash
./quickcache --port 6379 --max-keys 1000 --aof quickcache.aof
```

Connect with netcat:

```bash
nc localhost 6379
```

Try:

```text
PING
SET code:user123 849201 EX 300
GET code:user123
TTL code:user123
DELETE code:user123
GET code:user123
```

Example responses:

```text
PONG
OK
VALUE 849201
TTL 299
OK
NOT_FOUND
```

## Concurrent Clients

The TCP server handles multiple clients with one thread per connected client. Cache operations are protected by an internal mutex so shared state remains safe across concurrent `GET`, `SET`, `DELETE`, `EXPIRE`, `TTL`, and `STATS` calls.

Run the concurrency smoke client while the server is running:

```bash
python benchmarks/concurrency_client.py --clients 8 --operations 100
```

On Windows, use the Python launcher if needed:

```powershell
py .\benchmarks\concurrency_client.py --clients 8 --operations 100
```

## Append-Only Persistence

QuickCache can append successful write commands to an append-only log file:

```bash
./quickcache --port 6379 --max-keys 1000 --aof quickcache.aof
```

If `--aof` is omitted, QuickCache uses `quickcache.aof` by default:

```bash
./quickcache
```

is equivalent to:

```bash
./quickcache --port 6379 --max-keys 1000 --aof quickcache.aof
```

The server writes these operations to the log:

```text
SET key value
SET key value EX seconds
DELETE key
EXPIRE key seconds
```

On startup, QuickCache replays the log before accepting clients, rebuilding in-memory state after a crash or restart. Replay does not re-append commands, so recovery does not duplicate the log.

TTL-bearing writes are stored with absolute expiration timestamps internally, such as `EXAT <epoch_seconds>` and `EXPIREAT <epoch_seconds>`. This avoids resetting TTL on restart. For example, a verification code that expired while the server was offline is skipped during replay instead of coming back with a fresh lifetime.

To start with an empty cache, stop the server and either delete the AOF file or use a new filename:

```bash
rm quickcache.aof
./quickcache --aof fresh-session.aof
```

On Windows:

```powershell
Remove-Item .\quickcache.aof
.\Debug\quickcache.exe --aof fresh-session.aof
```

## Current Status

The core in-memory cache engine, command parser, TTL expiration, LRU eviction, multithreaded TCP server, and append-only persistence are implemented. Deeper benchmarks are planned for later tasks.
