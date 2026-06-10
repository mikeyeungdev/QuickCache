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
- `get(key)` returning the stored value or no value when the key is missing
- `remove(key)` returning `Ok` when a key was deleted and `NotFound` when it was absent

Example behavior covered by tests:

```text
SET code:user123 849201
GET code:user123
DELETE code:user123
```

This is a local C++ API only. TCP networking and command parsing are planned for later tasks.

## Planned Commands

- `PING`
- `SET`
- `GET`
- `DELETE`
- `EXPIRE`
- `TTL`
- `STATS`

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
./quickcache
```

On Windows with multi-configuration generators, run:

```powershell
.\Debug\quickcache.exe
```

## Current Status

The core in-memory cache engine is implemented. TCP networking, command parsing, TTL expiration, persistence, and benchmarks are planned for later tasks.
