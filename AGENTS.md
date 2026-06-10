# QuickCache Agent Notes

- Keep the cache engine independent from socket code so it remains testable.
- Prefer C++17 standard library facilities over platform-specific APIs unless networking requires them.
- Preserve append-only persistence compatibility when changing commands.
- Run `cmake -S . -B build` and `cmake --build build` after implementation changes when possible.
