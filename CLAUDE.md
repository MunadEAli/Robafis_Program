# ColonyAFIS firmware - notes for Claude

Firmware for the Université Marie et Louis Pasteur robot in ROBAFIS 2026 (ColonyAFIS track).
Munad (project manager) is building it step by step and learning as he goes: explain what you change and why, in plain words.
The full plan (modules, desk tests, integration steps, sensor fusion) is in `docs/ColonyAFIS_onboard_software_plan.pdf`. Read the relevant section before starting a new module.

## This PC
- The C: drive is nearly full. Install and download everything to D:, never to C:.
- PlatformIO Core lives in `D:\Tools\platformio` (`PLATFORMIO_CORE_DIR`). The `pio` command is `D:\Tools\platformio\penv\Scripts\pio.exe`. It is not on PATH, so call it by full path or from the PlatformIO terminal.
- VS Code is portable in `D:\Tools\VSCode`.
- The PC test compiler is MSYS2 gcc (UCRT64, g++ 16.2) in `D:\Tools\msys64`, installed from the self-extracting archive, so nothing is on C:. `D:\Tools\msys64\ucrt64\bin` is on the user PATH. Update it with `D:\Tools\msys64\usr\bin\bash.exe -lc "pacman -Syu"` (set `MSYSTEM=UCRT64`). The user PATH from before the install is saved in `D:\Tools\setup\user-PATH-backup-before-msys2.txt`.

## Build and test
- Robot build: `pio run -e megapi`. Upload: `pio run -e megapi -t upload`. Serial monitor: `pio device monitor -b 115200`.
- PC unit tests: `pio test -e test` (Unity, native).
- A `sim` environment (PC simulation of the whole robot through `hal_sim.cpp`) is planned.
- Any change to logic in `src/core/` gets a test in `test/`.

## Hardware
- MegaPi (ATmega2560, 8 KB SRAM). Use the Makeblock library (added to `lib_deps` at the HAL step).
- Motors:
  - 2× 185 rpm 9 V encoder motors for the drive and 1× 86 rpm motor for the rack-and-pinion arm, on SLOT1–3.
  - Driven with `MeEncoderOnBoard`. Encoder ISRs attach on `getIntNum()`, RISING.
  - The gripper DC motor is on SLOT4.
- Sensors:
  - `MeLineFollower`: 0 = black, 1 = white.
  - Ultrasonic on the left side detects cubes; the measured width tells a cube from the opponent robot. Always call `distanceCm(60)` so the timeout stays around 3.5 ms.
  - Colour sensor on the arm.
  - MPU-6050 accelerometer/gyro at 0x68, DLPF_CFG = 3 (44 Hz).
  - Camera shutter.
- Communications: Bluetooth module on `Serial3`.
- The robot size is not final. Every dimension is a TBD value in `include/config.h`.
  - Limits: width ≤ 16 cm and turn radius ≤ 12.5 cm, which gives ≤ 9.6 cm from the axle at 16 cm width.

## Architecture rules
1. **All tunable numbers go in `include/config.h`** (namespace `cfg`). No magic numbers in modules.
2. **The HAL is the only code that touches hardware.**
   - `hal.h` declares the hardware functions.
   - `hal_megapi.cpp` implements them on the robot, and `hal_sim.cpp` implements them on the PC (link-time substitution).
   - Modules never include Makeblock or Arduino headers directly.
3. **Everything is non-blocking.** Each module has `init()` and `update(now)`. No `delay()` and no waiting loops in the main loop.
4. **SAFETY is the only module that writes motor PWM** (`safety::actuatorsApply()`). Other modules only request outputs.
5. **Pure logic lives in `src/core/`**, with no Arduino includes, so it compiles and is tested on the PC.
6. **SRAM is tight.** No `String`, no `new`/`malloc`, constant strings inside `F("...")`, and fixed-size buffers.

## Modules (21)
CFG, HAL, DIAG · COMM, MISSION, SAFETY · IMU, ODOM, DRIVE, LINE, NAV, SCAN, COLOR · ARM, GRIP, MANIP · WORLD, STRAT, TASK, AUTOTEST, RECOVERY

Robot behaviour:
1. It follows the colony line and the ultrasonic detects cubes.
2. The arm reaches the cube and the colour sensor reads it. The robot remembers which deposit types it has seen.
3. It spins 180° and places the cube in the matching zone.

The turn spots, zones and deposits are listed in `config.h` (team frame, cm).

## Position estimate (see the fusion section of the plan)
Position along the track uses a two-state filter (speed and accelerometer bias):
- the accelerometer predicts and the encoders correct;
- a slip gate rejects encoder readings that disagree too much;
- the speed is reset to zero when the robot is stopped (ZUPT).

Heading uses the gyro, pulled gently back towards the line (β ≈ 0.01), and re-learns the gyro bias when stopped (α ≈ 0.02).

Write equations in docs as LaTeX display blocks.

## Test levels and integration
- **L0**: unit tests on the PC.
- **L1**: simulation.
- **L2**: desk tests with real parts but no robot.
- **L3**: on the robot.
- Integration goes in the order I0–I9 from the plan.

## Deadlines
| Milestone | Date |
| --- | --- |
| Autonomous test working | 11 Dec 2026 |
| Code freeze | 14 Dec 2026 |
| Final dossier | 18 Dec 2026 |
| Audits | 8 Jan 2027 |
| Competition, Toulouse | 12–13 Jan 2027 |
