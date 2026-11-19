# Motor Selection Options & Upgrade Path (1-Wheel Balance Cube)

This document records the motor candidates evaluated in Simscape Multibody + Electrical for the 1-reaction-wheel Balance Cube proof-of-concept (`140 mm` acrylic square, `90 mm` OD rimmed brass flywheel, `~387 g` wheel), along with compatibility against the electronics in [Balance_cube.kicad_sch](file:///home/thanotos/BalanceCube/Schematics/Balance_cube.kicad_sch) (`ESP32 DevKit V1`, `MPU6050`, quadrature encoder `CH_A`/`CH_B`, `L298N` H-bridge on `+12V`, `3A` polyfuse).

---

## 1. Active Baseline Recommendation (Locked Into Codegen & Simulink)

Until you finalize hardware purchase, the active baseline in [params_1d_cube.m](file:///home/thanotos/BalanceCube/simulink/params_1d_cube.m), [defaultControllerConfig.m](file:///home/thanotos/BalanceCube/codegen/defaultControllerConfig.m), and the generated ESP32 C99 code is:

* **Recommended Part Class**: **12 V `JGB37-520` (or `37GB-520`) `6.25:1` Low-Ratio Brushed DC Gearmotor with Rear 11 PPR Quadrature Hall Encoder** *(also consider the **`10:1`** variant as a direct drop-in)*
* **Key Baseline Specs**:
  * **Mass**: `155 g` (motor + gearbox + rear Hall encoder)
  * **Rated Stall Torque (@ 12 V)**: `0.24 N*m` (`0.20 N*m` effective at `10.0 V` after L298N drop)
  * **No-Load Speed (@ 12 V)**: `1600 RPM` (`1333 RPM` effective at `10.0 V`)
  * **Effective Stall Current (@ 10.0 V)**: `2.79 A` — fits safely inside your schematic's `3 A` polyfuse (`Polyfuse`) and `L298N` short-term peak rating
  * **Quadrature Encoder Resolution**: `11 PPR` on motor shaft $\times 4\text{x} \times 6.25 = \mathbf{275\text{ ticks/rev}}$ at flywheel (`440 ticks/rev` for the `10:1` variant)
  * **Nonlinear 3D Simscape Performance**: Recovers up to **`14 deg`** initial tilt cleanly; survives `0.04 N*m` taps with `138 RPM` peak wheel excursion and `< 2.8 A` peak current.

> **Mechanical Build Tip (Gearbox Backlash)**: Use a **clamping hub** (or keyed/D-shaft hub with blue Loctite on the set screw) rather than a loose grub screw on a round shaft; keep gear ratio low (`6.25:1` or `10:1`, never `30:1+`) so backlash stays small enough for crisp LQR damping.

---

## 2. Full Comparison of Motor Options

All figures below come directly from [analyze_feasibility.m](file:///home/thanotos/BalanceCube/simulink/analyze_feasibility.m) on the `140 mm` acrylic square + `90 mm` (`387 g`) rimmed brass flywheel on a `12 V` rail:

| Option | Motor + Driver | Mass | Stall Torque (@12V) | No-Load Speed (@V_eff) | Stall Current (@V_eff) | Linear Catch Limit | `6 deg` Simscape Outcome | Fit with Current KiCad PCB |
|---|---|---:|---:|---:|---:|---:|---|---|
| **A (Active Baseline)** | **`JGB37-520 6.25:1` + L298N** | `155 g` | `0.24 N*m` | `1333 RPM` | `2.79 A` | **`14.98 deg`** (`14 deg` nonlin) | **Recovers cleanly** (`2.79 A` peak) | **Drop-in compatible** (`PWM/IN1/IN2`, `CH_A/CH_B`, `3 A` fuse) |
| **A2 (Alternative Ratio)** | **`JGB37-520 10:1` + L298N** | `~155 g` | `~0.35 N*m` | `~830 RPM` | `~2.3 A` | `~16 deg` | Recovers (`~440` 4x CPR) | **Drop-in compatible**; slightly higher gear ratio $\rightarrow$ slightly more backlash |
| **B (High Ratio)** | `JGB37-520 30:1` + L298N | `155 g` | `0.75 N*m` | `275 RPM` | `1.80 A` | `11.36 deg` | Recovers (`1.79 A` peak) | Fits PCB, **not recommended**: saturates at `275 RPM` and heavy backlash causes oscillation |
| **C (Direct Drive Brushed)** | `RS-555` / `RS-775` + MOSFET | `220–350 g` | `0.26–0.45 N*m` | `3750 RPM` | **`8.51 A`** | **`17.18 deg`** | Recovers, **exceeds `3 A` fuse** (`8.49 A`) | **Incompatible with L298N**: requires `>=10 A` MOSFET H-bridge (`DRV8874`, `BTS7960`, `MD10C`) + `10 A` fuse |
| **D (2/3-Wheel Target)** | `Nidec 24H` / Gimbal BLDC + FOC/MOSFET | `130 g` | `0.30 N*m` | `3933 RPM` | `10.30 A` (current-limited by ESC) | **`25.65 deg`** | Best recovery & zero backlash (`10.14 A` peak) | **Bypass L298N**: drive via PWM/Dir or FOC directly from `+12V` power bus |
| **E (Disqualified)** | `RS-385` Direct + L298N | `80 g` | `0.035 N*m` | `7500 RPM` | `2.29 A` | `3.08 deg` | **FAILS (Falls)** at `6 deg` | Too weak to catch gravity torque |

---

## 3. How to Update Simulink & ESP32 Code Once You Pick Your Final Motor

When your physical motor arrives, update its exact datasheet values in **two places** (or directly in C without even re-running MATLAB):

### Option 1 — Re-synthesize Gains & Regenerate C99 Automatically (Recommended)
1. Measure or read from the motor datasheet:
   * `motor_mass_kg` (motor + encoder weight in kg)
   * `motor_T_stall_Nm` (stall torque at `12 V` in `N*m`)
   * `motor_rpm_noload` (output shaft no-load speed at `12 V` in `RPM`)
   * `V_drop_driver_V` (`2.0` for L298N, `0.2` if you swap to a MOSFET driver)
   * `cpr4x` (4 $\times$ `PPR` $\times$ gear ratio at the flywheel shaft)
2. Update those values in [params_1d_cube.m](file:///home/thanotos/BalanceCube/simulink/params_1d_cube.m) and `cpr4x` in [defaultControllerConfig.m](file:///home/thanotos/BalanceCube/codegen/defaultControllerConfig.m), then run:
   ```matlab
   run('/home/thanotos/BalanceCube/codegen/build_esp32_controller.m');
   ```
   This re-runs LQR synthesis, verifies MEX + SIL against the test suite, and overwrites the C99 files in `firmware/balance_controller_c/`.

### Option 2 — Runtime Tuning Directly in ESP32 C Code
Because `BalanceCubeConfig_T` is passed as an argument (`const BalanceCubeConfig_T *cfgParams`) into `balanceCubeControllerStep(...)`, you can adjust `cfg.K[0..3]`, `cfg.cpr4x`, `cfg.VmaxEff`, and `cfg.Vdeadband` directly inside [app_controller.c](file:///home/thanotos/BalanceCube/firmware/src/app_controller.c) (or live over UART using `set k1..k4`, `set cpr`, `set db`, `set vmax`) and reflash the ESP32 immediately without regenerating C code.
