# ColonyAFIS onboard software

PlatformIO project for the ROBAFIS 2026 robot (MegaPi).

- `include/robot_design.h` - the robot's mechanical dimensions from the CAD (size, wheels, arm, sensor positions). Owned by the mechanical designer; see [docs/mechanical_interface.md](docs/mechanical_interface.md).
- `include/config.h` - every other tunable number (limits, track layout, motors, timing). Change values here only.
- `src/main.cpp` - current firmware (step 1: smoke test).
- `src/core/` - pure logic with no Arduino code, testable on the PC.
- `test/` - unit tests that run on the PC: `pio test -e test` (uses MSYS2 gcc from `D:\Tools\msys64\ucrt64\bin`, which must be on PATH). `test_robot_design` checks the mechanical design against the competition limits; add `-v` to see its report.

Every push to GitHub runs the design check, the PC tests and the robot build automatically (Actions tab: green tick = all good).

Build: PlatformIO check-mark button. Upload: PlatformIO arrow button. Serial monitor: plug button (115200 baud).
