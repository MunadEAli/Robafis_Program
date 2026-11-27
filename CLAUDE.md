# ColonyAFIS firmware - notes for Claude

Firmware for the Université Marie et Louis Pasteur robot in ROBAFIS 2026 (ColonyAFIS track).
Munad (project manager) is building it step by step and learning as he goes: explain what you change and why, in plain words.
The full plan (modules, desk tests, integration steps, sensor fusion) is in `docs/ColonyAFIS_onboard_software_plan.pdf`. Read the relevant section before starting a new module.

## This PC
- The C: drive is nearly full. Install and download everything to D:, never to C:.
- PlatformIO Core lives in `D:\Tools\platformio` (`PLATFORMIO_CORE_DIR`). The `pio` command is `D:\Tools\platformio\penv\Scripts\pio.exe`. It is not on PATH, so call it by full path or from the PlatformIO terminal.
- VS Code is portable in `D:\Tools\VSCode`.
- The PC test compiler (MSYS2 gcc in `D:\Tools\msys64`, PATH entry `D:\Tools\msys64\ucrt64\bin`) is not installed yet. Until it is, `pio test -e test` will fail.

## PRIORITY: real hardware first
IMPORTANT: Munad wants code that runs on the MegaPi with real sensors and motors connected, tested physically on the desk while the robot is being built. Simulation (`hal_sim.cpp`, a `sim` env) comes later, only if time allows. Do not start simulation work unless he asks.

Each part gets a **desk-test program**:
- Create a PlatformIO env `hw_<part>` that `extends = env:megapi` and uses `build_src_filter` to build `src/hwtest/<part>.cpp` plus the shared code. Do not build `src/main.cpp` in these envs.
- The program prints readings to Serial and, once Bluetooth works, to Bluetooth too.
- It accepts simple single-letter serial commands, for example motor speed, open/close or start/stop, so Munad can test by hand.
- Upload with `pio run -e hw_<part> -t upload`.
- Port and slot numbers go in `config.h`. Ask Munad where each part is plugged in. Never guess.

Desk-test order:
1. Serial and Bluetooth
2. Ultrasonic
3. Line follower
4. Colour sensor (calibrate on real cubes)
5. Gyro and accelerometer
6. Drive motors and encoders (measure cm per pulse)
7. Arm motor (homing, position)
8. Gripper

Then combine the parts in this order:
1. Line following.
2. Detecting cubes while driving.
3. Picking a cube.
4. Spinning 180°.
5. Placing the cube.
6. The full auto-test scenario.

PC unit tests stay for the pure maths only (geometry, filters, strategy). They never block hardware work.

## Git branches
- Day-to-day work happens on `dev`. Commit and push there, including steps that are still in progress.
- `main` only gets finished steps: merge `dev` into `main` when a step's "done when" is met, with the measured numbers in the commit message.
- Raw monitor logs (`logs/device-monitor-*.log`, written by the monitor's log2file filter) are not committed. Keep the useful ones as `tests/results/<date>_<part>.csv`.

## Build and test
- Robot build: `pio run -e megapi`. Upload: `pio run -e megapi -t upload`. Serial monitor: `pio device monitor -b 115200`.
- PC unit tests: `pio test -e test` (Unity, native). Add `-v` to see the design report.
- GitHub Actions (`.github/workflows/check.yml`) runs `pio test -e test -v` and `pio run -e megapi` on every push and pull request.
- Any change to logic in `src/core/` gets a test in `test/`.

## Hardware
- MegaPi (ATmega2560, 8 KB SRAM). Use the Makeblock library (added to `lib_deps` at the HAL step).
- Motors:
  - 2× 185 rpm 9 V encoder motors for the drive and 1× 86 rpm motor for the rack-and-pinion arm, on SLOT1–3.
  - Driven with `MeEncoderOnBoard`. Encoder ISRs attach on `getIntNum()`, RISING.
  - The gripper DC motor is on SLOT4.
- Sensors:
  - `MeLineFollower`: 0 = black, 1 = white.
  - Ultrasonic on the left side detects cubes; the measured width tells a cube from the opponent robot. It is on port 6 of the RJ25 shield, signal on pin 63 (A9), confirmed in desk test 4. Port 5 gave no echo on either pin; suspect the shield's 2×2 corner, so don't use port 5 until that is checked.
  - Do not use Makeblock's `distanceCm(60)`. In library 3.31 it ignores the 60 and waits for the whole echo pulse (tens of ms with nothing in range), and it returns the previous value if called again within 23 ms. Read the pin directly with `pulseIn(pin, HIGH, cfg::US_TIMEOUT_US)` (3.7 ms cap), as `src/hwtest/ultrasonic.cpp` does.
  - Desk test 4 (`tests/results/2026-11-27_ultrasonic.csv`): a 3 cm cube looks 7–9 cm wide at 8–14 cm, and a 16 cm flat object looks ~23 cm wide at 10 cm. Readings at an object's edges come out up to ~5 cm too long, and single stray far echoes (an arm, the room) happen. Thresholds are in `cfg::SCAN_*`.
  - Sensor ports 5–8 are on the "MegaPi Shield for RJ25", stacked on the MegaPi's left grey/red/yellow (GND/5V/SIG) pin rows. Port-to-pin table: `cfg::MEGAPI_PORT_PIN1/2`.
  - Colour sensor on the arm.
  - MPU-6050 accelerometer/gyro at 0x68, DLPF_CFG = 3 (44 Hz).
  - Camera shutter.
- Communications: Makeblock dual-mode Bluetooth module in the MegaPi's onboard wireless socket, on `Serial3` at 115200 (confirmed in desk test 1, `hw_link`).
  - While no phone or laptop is connected, the module answers everything the MegaPi sends with `+ERROR01`, so code must never echo unknown Bluetooth bytes back to Bluetooth.
  - Phones and laptops connect over BLE, not classic Bluetooth: the device is `Makeblock_LE…`, with a custom profile of service `0000ffe1-…`, read (notify) `0000ffe2-…` and write `0000ffe3-…` (all `-0000-1000-8000-00805f9b34fb`). On Android, use Serial Bluetooth Terminal and long-press the device to set the custom profile.
  - MegaPi USB is COM8 on Munad's PC (CH340). COM3 is the PC's Intel port, never the board.
  - Still to do for step 3: measure the STOP round-trip time from the phone (plan: under 0.5 s).
- The robot size is not final. Every mechanical dimension (body, wheels, arm, sensor positions) is a TBD value in `include/robot_design.h`. The mechanical teammate owns that file; `config.h` includes it, so the values are still `cfg::`. `test/test_robot_design` checks them against the limits below, and `docs/mechanical_interface.md` explains them to the teammate. The arm numbers refer to the grip point (the middle of the jaws).
  - Limits: width ≤ 16 cm and turn radius ≤ 12.5 cm, which gives ≤ 9.6 cm from the axle at 16 cm width.
  - These limits include the retracted arm and anything sticking out.
  - Arm reach from the robot's side: the deposit cube centre is 20 cm from the colony line, so reach ≥ 20 − W/2; the zone centre is 17.5 cm from the line.

## Rules that affect the code (CDC v1.1)
Auto-test scenario:
- Fetch the water cube (blue) and place it in Habitat in under 2 minutes.

During a match:
- Stay at most 20 s inside a deposit.
- Stop in under 3 s on an emergency-stop command, and wait for an explicit resume.
- Touching the outer boundary line counts as leaving the field, and the robot must then trigger its own emergency stop.
- Never enter the opponent's colony or a deposit occupied by the other robot.
- Report the detected resource, the picked resource, the target zone and the robot state to the governor (Bluetooth).

## Architecture rules
1. **All tunable numbers go in `include/config.h`** (namespace `cfg`). Mechanical dimensions go in `include/robot_design.h`, which `config.h` includes. No magic numbers in modules.
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
- **L2 desk tests on real hardware come first** (see PRIORITY above), then **L3** on the assembled robot.
- **L0** PC unit tests only for the pure maths.
- **L1** simulation is deferred.
- Integration still follows the order I0–I9 from the plan, but skip the simulation parts for now.

## Deadlines
| Milestone | Date |
| --- | --- |
| Autonomous test working | 11 Dec 2026 |
| Code freeze | 14 Dec 2026 |
| Final dossier | 18 Dec 2026 |
| Audits | 8 Jan 2027 |
| Competition, Toulouse | 12–13 Jan 2027 |
