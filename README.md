# ColonyAFIS onboard software

PlatformIO project for the ROBAFIS 2026 robot (MegaPi).

- `include/config.h` - every tunable number (robot size, wheels, track layout). Change values here only.
- `src/main.cpp` - current firmware (step 1: smoke test).
- `src/core/` - pure logic with no Arduino code, testable on the PC.
- `test/` - unit tests that run on the PC: `pio test -e test` (uses MSYS2 gcc from `D:\Tools\msys64\ucrt64\bin`, which must be on PATH).

Build: PlatformIO check-mark button. Upload: PlatformIO arrow button. Serial monitor: plug button (115200 baud).
