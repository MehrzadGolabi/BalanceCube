# 1-Wheel Balance Cube: Simulink Controller Design & MATLAB Coder / Embedded Coder MCU Plan

This document specifies the architecture, sensor-processing pipeline, safety state machine, and C99 code-generation workflow to deploy the simulated 1-reaction-wheel Balance Cube controller from Simulink / MATLAB onto the **ESP32 DevKit V1** target defined in [Balance_cube.kicad_sch](file:///home/thanotos/BalanceCube/Schematics/Balance_cube.kicad_sch).

---

## 1. Hardware-to-Simulink Interface Mapping (From `Balance_cube.kicad_sch`)

| Hardware Block | KiCad Schematic Nets / Part | Physical Signal & Direction | Simulink / Codegen Port Mapping |
|---|---|---|---|
| **MCU** | `ESP32_DevKit_V1_DOIT_30GPIOs` | Xtensa LX6 dual-core @ 240 MHz, hardware single-precision (`float32`) FPU | Target execution (`single` data types; Core 1 deterministic loop) |
| **IMU** | `MPU6050` (`SDA`, `SCL`, `IMU_INT`) | 3-axis accel + 3-axis gyro @ 400 kHz I2C | `ax_mps2`, `ay_mps2`, `gz_rads` (`single`) |
| **Encoder** | `Encoder` (`CH_A`, `CH_B`) | 4x quadrature pulses via ESP32 hardware `PCNT` | `enc_ticks_delta` (`int32`) $\rightarrow$ `wheel_w_rads` (`single`) |
| **Motor Driver** | `L298N` (`PWM`, `IN1`, `IN2`) on `+12V` + `Polyfuse` (`3A`) | H-bridge signed voltage command $V_{\text{cmd}} \in [-10\,\text{V}, +10\,\text{V}]$ | `pwm_duty` (`single` $\in [0, 1]$), `in1_high` (`logical`), `in2_high` (`logical`) |
| **User Feedback** | `WS2812B` (`LED_DATA`), `Buzzer`, SPI `OLED` (`CS`, `DC`, `RES`, `D0`, `D1`) | Mode indicator LED, calibration chirp, live telemetry | `state_mode` (`uint8`: 0=Calib, 1=Idle, 2=Balance, 3=Fault) |

### Control Loop Rate Selection: `200 Hz` (`Ts = 0.005 s`) vs `500 Hz` (`Ts = 0.002 s`)
* The baseline 140 mm acrylic + 90 mm brass flywheel has an open-loop unstable pole $p = 10.20\text{ rad/s}$ ($\tau_{\text{fall}} = 98\text{ ms}$, bandwidth $\approx 1.6\text{ Hz}$).
* Sampling at **`200 Hz` (`Ts = 0.005 s`)** is $> 120\times$ faster than $1.6\text{ Hz}$ (adding $< 1.5^\circ$ phase lag) while leaving generous CPU headroom inside a 5 ms slot:
  * MPU6050 14-byte I2C burst read @ 400 kHz: `~0.35 ms`
  * Complementary/Kalman filter + 4-state LQR (`single` math): `< 0.02 ms`
  * Optional upgrade to `500 Hz` (`Ts = 0.002 s`) once verified on bench.

---

## 2. Simulink Controller Architecture (Upgrading Ideal Simscape Signals to Real Sensors)

Inside `BalanceCube_1D.slx`, the `Controller` subsystem currently receives ideal continuous `theta_rad`, `theta_dot_rads`, and `wheel_w_rads` signals. Before generating MCU firmware, build a target-ready **`BalanceCube_Controller.slx`** (or test the equivalent `MATLAB Function` block directly inside `BalanceCube_1D.slx`) containing three sub-blocks:

```
  [ax, ay, gz] ──► 1. State Estimator ──► [theta_hat, theta_dot, wheel_w] ──► 2. Mode Supervisor
  [enc_delta]  ──► (45° IMU rot + CF)                                          (CALIB / IDLE / BAL / FAULT)
                                                                                      │
  [PWM, IN1, IN2] ◄── 3. LQR + Anti-Windup + L298N Deadzone Comp ◄────────────────────┘
```

### 2.1 Subsystem 1 — Sensor Calibration & State Estimation
1. **45° Diamond Frame Rotation (Critical Geometry Detail)**:
   * Because the square plate stands on one corner ($45^\circ$ diamond), if the `MPU6050` PCB is mounted parallel to a square edge rather than horizontally, its raw $(a_{x,\text{raw}}, a_{y,\text{raw}})$ accelerometer axes are rotated by $45^\circ$ from the vertical diagonal:
     $$\begin{bmatrix} a_x \\ a_y \end{bmatrix} = \begin{bmatrix} \cos 45^\circ & -\sin 45^\circ \\ \sin 45^\circ & \cos 45^\circ \end{bmatrix} \begin{bmatrix} a_{x,\text{raw}} \\ a_{y,\text{raw}} \end{bmatrix}$$
   * Apply a static mechanical zero trim $\theta_{\text{mount\_trim}}$ (measured once on bench or auto-trimmed by the 4th integrator state).
2. **Startup Gyro Bias Calibration**:
   * Average `gz` over `400` samples (`2.0 s` at `200 Hz`) while resting stationary (`CALIBRATING` mode) to subtract `gz_bias`.
3. **Tilt Complementary Filter (Zero-Lag Gyro + Drift-Free Accel)**:
   * $\hat{\theta}_{\text{dot}}[k] = g_z[k] - g_{z,\text{bias}}$
   * $\theta_{\text{acc}}[k] = \text{atan2}(a_x[k], a_y[k]) - \theta_{\text{mount\_trim}}$
   * $\hat{\theta}[k] = \alpha_{\text{cf}} \big(\hat{\theta}[k-1] + \hat{\theta}_{\text{dot}}[k] T_s\big) + (1 - \alpha_{\text{cf}}) \theta_{\text{acc}}[k]$, with $\alpha_{\text{cf}} = 0.992$ ($\tau_{\text{cf}} \approx 0.62\text{ s}$).
   * *Why Complementary Filter (or 2-state steady-state Kalman)?* During dynamic wheel acceleration, the accelerometer sees tangential centripetal/reaction noise; $\alpha_{\text{cf}} = 0.992$ rejects reaction-wheel vibration while eliminating gyro drift.
4. **Quadrature Encoder Velocity + LPF**:
   * Read 4x `PCNT` ticks $\Delta N$ and reset counter each $T_s$:
     $$\omega_{w,\text{raw}}[k] = \frac{2\pi \,\Delta N}{\text{CPR}_{\text{4x}} \, T_s}$$
   * Pass through a 1st-order `single`-precision discrete low-pass filter ($f_c = 30\text{ Hz}$, $\beta = \frac{2\pi f_c T_s}{1 + 2\pi f_c T_s} \approx 0.485$):
     $$\hat{\omega}_w[k] = (1 - \beta)\hat{\omega}_w[k-1] + \beta \omega_{w,\text{raw}}[k]$$
   * **Note on motor gearing**: If using a `6.25:1` gearbox (`JGB37-520`), mount the flywheel rigidly without play (e.g., clamping hub + Loctite) and verify encoder CPR at the motor shaft vs output shaft (`output_rad_s = motor_rad_s / gear_ratio`).

### 2.2 Subsystem 2 — Mode Supervisory State Machine
Implement in Stateflow or `persistent` enum inside the step function:
* **`MODE_CALIB (0)`**: Runs for `400` ticks (`2.0 s`). Outputs `PWM = 0`. Beeps `Buzzer` once bias calibration finishes.
* **`MODE_IDLE (1)`**: Waits safely with `PWM = 0`, `int_w = 0`, `WS2812B` = Blue/Yellow pulsing. Transitions to `MODE_BALANCE` only when the user holds the frame upright within $|\hat{\theta}| < 3.0^\circ$ and $|\hat{\theta}_{\text{dot}}| < 0.35\text{ rad/s}$ for `20` consecutive ticks (`100 ms`).
* **`MODE_BALANCE (2)`**: Executes 4-state discrete LQR (`WS2812B` = Solid Green).
* **`MODE_FAULT (3)`**: Trips immediately (`PWM = 0`, `IN1 = 0`, `IN2 = 0`, `WS2812B` = Red) if $|\hat{\theta}| > 30^\circ$ (`catch_limit_rad`) or wheel speed stays $> 95\%$ of `w_max_eff` for $> 0.5\text{ s}$. Recovers automatically to `MODE_IDLE` once returned near upright.

### 2.3 Subsystem 3 — 4-State Discrete LQR + Anti-Windup + L298N Deadzone Compensation
* **4-State Control Law (`single` precision)**:
  $$V_{\text{lqr}} = -\Big(K_1 \hat{\theta} + K_2 \hat{\theta}_{\text{dot}} + K_3 \hat{\omega}_w + K_4 I_{\omega_w}\Big)$$
  where baseline 200 Hz gains from [params_1d_cube.m](file:///home/thanotos/BalanceCube/simulink/params_1d_cube.m) are:
  `K_lqr = single([-128.852, -13.393, -0.3655, -0.1456])`.
* **Conditional Integration Anti-Windup**:
  Only integrate `int_w` when $V_{\text{lqr}}$ is **not saturated** (`|V_lqr| < V_max_eff_V`) and clamp `int_w` within `[-25, +25] rad`. This stops windup during large perturbations while preserving zero-RPM steady-state convergence under small CoM imbalances.
* **L298N Darlington Drop & Motor Stiction Compensation**:
  L298N exhibits ~`1.8–2.2 V` BJT saturation drop plus brush/bearing stiction (~`0.4–0.8 V`). Map commanded voltage to PWM duty via:
  ```matlab
  if abs(V_cmd) < single(0.05)
      V_pwm = single(0);
  else
      % Feedforward stiction/driver deadband compensation (tunable on bench)
      V_pwm = V_cmd + sign(V_cmd) * V_deadband_V;
  end
  duty = min(single(1.0), abs(V_pwm) / V_supply_V);
  ```
  And set H-bridge direction pins matching `Balance_cube.kicad_sch`:
  * `V_cmd > +0.05`: `IN1 = true, IN2 = false, PWM = duty`
  * `V_cmd < -0.05`: `IN1 = false, IN2 = true, PWM = duty`
  * Otherwise: `IN1 = false, IN2 = false, PWM = 0` (coast/disabled)

---

## 3. Code Generation Workflows (R2025b Verified)

Both **MATLAB Coder** (requested) and **Simulink Embedded Coder** are installed in this MATLAB R2025b environment. Below is the exact workflow for **MATLAB Coder** (ideal for clean integration into an Arduino / PlatformIO / ESP-IDF project on ESP32).

### Step 3.1 Author the `%#codegen` Entry-Point Function
Create `codegen_src/balanceCubeControllerStep.m` using **only `single` (`float32`), `int32`, `uint8`, and `logical`** types (ESP32's hardware FPU accelerates `float32`, whereas `double` triggers slow software emulation!):

```matlab
function [pwmDuty, in1High, in2High, modeOut, thetaHatDeg] = balanceCubeControllerStep( ...
    axMps2, ayMps2, gzRads, encTicksDelta, cfgParams) %#codegen
%BALANCECUBECONTROLLERSTEP Fixed-step 200 Hz controller for 1D Balance Cube (ESP32).

    persistent isInit calibCount gzSum gzBias thetaHat omegaWFilt intWheelW ...
               uprightCount modeState

    if isempty(isInit)
        isInit = true;
        calibCount = uint16(0);
        gzSum = single(0);
        gzBias = single(0);
        thetaHat = single(0);
        omegaWFilt = single(0);
        intWheelW = single(0);
        uprightCount = uint16(0);
        modeState = uint8(0); % 0=CALIB, 1=IDLE, 2=BALANCE, 3=FAULT
    end

    Ts = cfgParams.Ts;

    % 1. Rotate accelerometer 45 deg if IMU aligns with square edges
    c45 = single(0.70710678);
    axRot = c45 * (axMps2 - ayMps2);
    ayRot = c45 * (axMps2 + ayMps2);
    thetaAcc = atan2(axRot, ayRot) - cfgParams.mountTrimRad;

    % 2. Wheel velocity from 4x quadrature encoder ticks + 30 Hz LPF
    omegaWRaw = (single(2.0 * pi) * single(encTicksDelta)) / (cfgParams.cpr4x * Ts);
    beta = cfgParams.encLpfBeta;
    omegaWFilt = (single(1.0) - beta) * omegaWFilt + beta * omegaWRaw;

    % 3. Complementary tilt filter
    thetaDot = gzRads - gzBias;
    alpha = cfgParams.cfAlpha;
    thetaHat = alpha * (thetaHat + thetaDot * Ts) + (single(1.0) - alpha) * thetaAcc;

    Vcmd = single(0);

    % 4. Mode state machine
    switch modeState
        case uint8(0) % CALIBRATING
            thetaHat = thetaAcc;
            gzSum = gzSum + gzRads;
            calibCount = calibCount + uint16(1);
            if calibCount >= uint16(400) % 2.0 s @ 200 Hz
                gzBias = gzSum / single(400);
                modeState = uint8(1);
            end

        case uint8(1) % IDLE_WAIT
            intWheelW = single(0);
            if abs(thetaHat) < cfgParams.engageAngleRad && abs(thetaDot) < single(0.35)
                uprightCount = uprightCount + uint16(1);
                if uprightCount >= uint16(20) % 100 ms steady
                    modeState = uint8(2);
                end
            else
                uprightCount = uint16(0);
            end

        case uint8(2) % BALANCING (4-state discrete LQR)
            if abs(thetaHat) > cfgParams.catchLimitRad
                modeState = uint8(3);
                intWheelW = single(0);
                Vcmd = single(0);
            else
                Vlqr = -(cfgParams.K(1) * thetaHat + ...
                         cfgParams.K(2) * thetaDot + ...
                         cfgParams.K(3) * omegaWFilt + ...
                         cfgParams.K(4) * intWheelW);

                % Conditional anti-windup
                if abs(Vlqr) < cfgParams.VmaxEff
                    intWheelW = intWheelW + omegaWFilt * Ts;
                    intWheelW = min(cfgParams.intWMax, max(-cfgParams.intWMax, intWheelW));
                end
                Vcmd = min(cfgParams.VmaxEff, max(-cfgParams.VmaxEff, Vlqr));
            end

        otherwise % MODE_FAULT (3)
            intWheelW = single(0);
            Vcmd = single(0);
            if abs(thetaHat) < cfgParams.engageAngleRad && abs(thetaDot) < single(0.35)
                modeState = uint8(1);
                uprightCount = uint16(0);
            end
    end

    % 5. H-bridge PWM & direction mapping (L298N: PWM, IN1, IN2)
    if abs(Vcmd) < single(0.05)
        pwmDuty = single(0);
        in1High = false;
        in2High = false;
    else
        Vcomp = Vcmd + sign(Vcmd) * cfgParams.Vdeadband;
        pwmDuty = min(single(1.0), abs(Vcomp) / cfgParams.Vsupply);
        in1High = (Vcmd > single(0));
        in2High = (Vcmd < single(0));
    end

    modeOut = modeState;
    thetaHatDeg = thetaHat * single(57.29578);
end
```

### Step 3.2 Validate inside Simulink + Run MATLAB Coder Pipeline (`MEX -> SIL -> C99 Lib`)
1. **Plant-in-the-Loop Validation inside `BalanceCube_1D.slx`**:
   * Replace `Controller` contents with a `MATLAB Function` block calling `balanceCubeControllerStep(...)` (with synthetic 4x encoder quantization + IMU bias/noise injected in `Plant`) to verify exact closed-loop performance against the ideal continuous-sensor baseline.
2. **Codegen Readiness Screener** *(always capture output struct)*:
   ```matlab
   res = coder.screener('balanceCubeControllerStep');
   disp(res.UnsupportedCalls);
   disp(res.Messages);
   ```
3. **Define Fixed Input Types & Build MEX Verification**:
   ```matlab
   cfgType = struct( ...
       'Ts', single(0.005), ...
       'mountTrimRad', single(0), ...
       'cpr4x', single(44 * 6.25), ... % verify exact shaft CPR
       'encLpfBeta', single(0.485), ...
       'cfAlpha', single(0.992), ...
       'engageAngleRad', single(deg2rad(3.0)), ...
       'catchLimitRad', single(deg2rad(30.0)), ...
       'K', single(K_lqr), ...
       'VmaxEff', single(10.0), ...
       'Vdeadband', single(0.5), ...
       'Vsupply', single(12.0), ...
       'intWMax', single(25.0));

   argsExample = {single(0), single(9.81), single(0), int32(0), coder.Constant(cfgType)};
   codegen('balanceCubeControllerStep', '-args', argsExample);
   ```
   Verify MEX output matches interpreted MATLAB within `< 1e-6` (`AbsTol`) via a `coder.runTest` test script.
4. **Software-in-the-Loop (SIL) + Embedded Bare-Metal Static C99 Generation**:
   ```matlab
   cfg = coder.config('lib', 'ecoder', true);
   cfg.TargetLang = 'C';
   cfg.EnableDynamicMemoryAllocation = false;   % Zero malloc/free for MCU safety
   cfg.StackUsageMax = 512;                     % Conservative ESP32 task stack budget
   cfg.HardwareImplementation.ProdHWDeviceType = 'Generic->32-bit Embedded Processor';
   cfg.HardwareImplementation.ProdBitPerFloat = 32;
   cfg.HardwareImplementation.ProdBitPerDouble = 64;

   % Optional SIL check first:
   % cfgSil = copy(cfg); cfgSil.VerificationMode = 'SIL';

   outDir = fullfile(pwd, 'codegen_esp32');
   codegen('balanceCubeControllerStep', '-config', cfg, '-args', argsExample, '-d', outDir);
   ```
   * **Alternative Path (Simulink Embedded Coder `ert.tlc`)**: If you keep the controller strictly in Simulink blocks (`BalanceCube_Controller.slx` with `FixedStepDiscrete` solver at `Ts = 0.005` and `single` I/O types), configure `ert.tlc` (`Target="ert"`, `Language="C"`, `Interface="Nonreusable function"`, `Objective="Speed"`) to generate `BalanceCube_Controller_initialize()` and `BalanceCube_Controller_step()`.

---

## 4. ESP32 Firmware Integration Skeleton (`Arduino / ESP-IDF / PlatformIO`)

Copy the generated `.c` and `.h` files from `codegen_esp32/` (excluding `examples/main.c`) into your ESP32 `src/` / `include/` folder and call them from a **dedicated Core-1 deterministic loop** (keeping Wi-Fi/OLED/Serial telemetry on Core 0 so I2C/control jitter stays under `50 us`):

```c
#include "balanceCubeControllerStep.h"
#include "balanceCubeControllerStep_initialize.h"

void control_task_core1(void *arg) {
    balanceCubeControllerStep_initialize();
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t periodTicks = pdMS_TO_TICKS(5); // 200 Hz

    for (;;) {
        vTaskDelayUntil(&lastWake, periodTicks);

        float ax, ay, gz;
        mpu6050_read_burst(&ax, &ay, &gz);      // 400 kHz I2C burst
        int32_t encDelta = pcnt_read_and_clear();

        float pwmDuty = 0.0f, thetaDeg = 0.0f;
        boolean_T in1 = false, in2 = false;
        uint8_T mode = 0;

        balanceCubeControllerStep(ax, ay, gz, encDelta,
                                  &pwmDuty, &in1, &in2, &mode, &thetaDeg);

        gpio_set_level(PIN_IN1, in1 ? 1 : 0);
        gpio_set_level(PIN_IN2, in2 ? 1 : 0);
        ledc_set_duty_and_update(PWM_CH, (uint32_t)(pwmDuty * 1023.0f));
        update_status_led_oled(mode, thetaDeg); // push to Core 0 queue
    }
}
```

### Bench Commissioning Sequence (Safety First!)
1. **Sensor Polarity Check (Motor Power Disconnected)**: Tilt the square in $+ \theta$ (CCW looking from flywheel side) and confirm `thetaHatDeg > 0` and `thetaDot > 0`. Spin the flywheel CCW (`+`) by hand and verify `encDelta > 0`.
2. **Motor Sign Check (Low Fixed PWM)**: Command `+15%` duty (`IN1=1, IN2=0`) while holding frame still; confirm wheel accelerates in `+` (`encDelta > 0`) — guaranteeing negative reaction torque on the frame!
3. **Engage Balancing**: Hold frame upright inside `|theta| < 3 deg` until `WS2812B` turns Green, then gently release.

---

## 5. Scaling Path: 1-Wheel PoC $\rightarrow$ 2-Wheel $\rightarrow$ 3-Wheel Full Cubli

| Stage | Mechanical / Actuation Reality | Recommended Hardware Upgrade Beyond 1-Wheel L298N PoC |
|---|---|---|
| **Stage 1: 1-Wheel PoC (This Repo)** | Pillow-block bearing constrains motion to 1-DOF; 12V bench supply tether OK | `JGB37-520 6.25:1` brushed motor + `90 mm` rimmed brass flywheel (`~390 g`) works within existing `L298N` + `3 A` polyfuse (`~14°` catch window). |
| **Stage 2: 2-Wheel Edge/Corner** | 2 orthogonal wheels share frame weight ($\sqrt{2}\times$ more frame mass; two axes must decouple) | Replace `L298N` (`2 V` drop, `2 A` limit) with MOSFET drivers (`DRV8874` / `BTS7960`) OR switch to brushless reaction motors (`Nidec 24H` w/ built-in FOC/PWM driver) to eliminate gearbox backlash. |
| **Stage 3: 3-Wheel Full Cubli** | Free 3D vertex balance requires onboard battery + IMU quaternion attitude + 3 orthogonal reaction wheels + servo snap-brakes if 45° self-jump-up is desired | **3S/4S high-discharge LiPo**, 3x brushless outrunner/gimbal motors with field-oriented control (FOC / SimpleFOC / ODrive / Nidec 24H), and spring-loaded mechanical wheel brakes for floor-to-vertex jump-up. |
