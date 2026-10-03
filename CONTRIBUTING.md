# Contributing to WattLab Nexus

Thanks for your interest. This document describes how to work on the project
without breaking the verified baseline.

## Ground rules

1. **Never fake hardware results.** If a feature cannot be tested without a
   board, mark it `NOT TESTABLE` / `NOT VERIFIED`. Do not claim a hardware pass.
2. **Keep the host build green.** The host test suite is the regression
   baseline. A change that breaks it must not be merged.
3. **Respect the layering.** UI and services must not call hardware drivers
   directly. Go through the abstraction (e.g. `nexus_storage`, `nexus_time`).
4. **No slow I/O on high-priority tasks.** SD writes go through the storage
   task; logging enqueues.
5. **No hardcoded user paths.** Use paths relative to the repository root.

## Development setup

```batch
scripts\setup.bat
```

Installs/verifies Arduino CLI, the STM32 core and libraries, and a C++17
compiler. Then:

```batch
scripts\test.bat            :: host tests
scripts\run-simulator.ps1   :: simulator
scripts\build.bat           :: firmware
scripts\lint.bat            :: static analysis
scripts\test-all.bat        :: everything + summary
```

## Source layout

- Canonical firmware sources: `firmware/src` and `firmware/include`.
- The Arduino sketch folder `firmware/wattlab_nexus` is **generated** by
  `scripts/sync-sketch.ps1` (mirrored sources are git-ignored). Edit the
  canonical files; never edit the mirrored copies.
- Host-only code lives in `host/` (simulator, mocks).
- Tests live in `tests/unit` and `tests/integration`.

## Adding a module

1. Add `firmware/include/nexus_<name>.h` and `firmware/src/nexus_<name>.cpp`.
2. Keep the module hardware-independent where possible; isolate hardware behind
   a backend/interface guarded by `ARDUINO_ARCH_STM32`.
3. Add a unit test `tests/unit/test_<name>.cpp` exposing
   `int test_<name>(void)` (return `0` on success).
4. Register the test in `tests/unit/test_main.cpp`.
5. Add the new sources to `scripts/run-tests.ps1` (and `run-simulator.ps1` /
   `CMakeLists.txt` if applicable).
6. Run `scripts\test-all.bat`.

## Code style

- C++17. 4-space indentation for C++ (the existing files use 4 spaces).
- Meaningful names; no unexplained magic numbers.
- Comments explain **why**, not what.
- Prefer fixed-size buffers; avoid dynamic allocation in real-time paths.
- Handle every error; return `nexus_err_t` from fallible functions.
- Address all compiler warnings (`-Wall -Wextra` is used by the test runner).

## Commit messages

Use a clear imperative summary, e.g.:

```
storage: add QSPI backend scaffolding
time: reject implausible RTC years
docs: clarify RTC persistence limitation
```

## Testing expectations

| Change type | Required |
|-------------|----------|
| Pure logic | Unit test |
| Storage/time/logging | Unit test + host backend |
| Cross-module flow | Integration test (`test_alpha_flow.cpp`) |
| Hardware binding | Compile-verified + documented as pending |

## Reporting issues

Include:
- version (`nexus_version.h`),
- whether a board was attached,
- exact commands and serial output,
- for storage issues: card model/capacity and filesystem type.

## License

By contributing you agree your contributions are licensed under the MIT License
(see `LICENSE`).
