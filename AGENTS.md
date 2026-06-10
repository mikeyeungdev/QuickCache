# AGENTS.md

Guidance for future Codex tasks in the QuickCache repository.

## Project

QuickCache is a C++ Redis-style in-memory cache for short-lived backend data such as ticket queue positions, verification codes, and inventory holds.

## Build

From the repository root:

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

On Windows PowerShell:

```powershell
New-Item -ItemType Directory -Force build
Set-Location build
cmake ..
cmake --build .
```

## Run

From the build directory:

```bash
./quickcache
```

On Windows with multi-configuration generators:

```powershell
.\Debug\quickcache.exe
```

## Test

```bash
ctest --output-on-failure
```

On Windows with Visual Studio generators:

```powershell
ctest -C Debug --output-on-failure
```

## Notes

- Keep future implementation changes scoped to the requested task.
- Prefer readable C++17 and simple CMake targets.
- Keep cache logic independent from networking and command parsing.
