# KiCad Schematic Audit & Fixes (`Balance_cube.kicad_sch`)

Before laying out [Balance_cube.kicad_pcb](file:///home/thanotos/BalanceCube/Schematics/Balance_cube.kicad_pcb), a `kicad-cli` ERC and netlist inspection of [Balance_cube.kicad_sch](file:///home/thanotos/BalanceCube/Schematics/Balance_cube.kicad_sch) uncovered five hardware-critical circuit bugs. All five have been fixed in the schematic (`Errors: 0` in `kicad-cli sch erc`), and standard KiCad 9 footprints were assigned across the BOM.

![Fixed KiCad Schematic Preview](file:///home/thanotos/BalanceCube/docs/schematic_fixed_preview.png)

---

## 1. Summary of Hardware Bugs Fixed

| # | Original Issue | Physical Consequence | Fix Applied in `Balance_cube.kicad_sch` |
|---|---|---|---|
| **1** | `L298N` (`U2`) `Vs` (Pin 4) and `Vss` (Pin 9) swapped (`+5V` on `Vs`, `+12V` on `Vss`) | Exceeds `Vss` `7 V` absolute max rating, destroying the `L298N` logic rail (and potentially back-feeding the ESP32) | Swapped power ports: `#PWR013` $\rightarrow$ `+5V` (`Vss`), `#PWR016` $\rightarrow$ `+12V` (`Vs`). |
| **2** | `U2` `SENSE_A` (Pin 1) & `SENSE_B` (Pin 15) marked `no_connect` | Floating lower emitter return of H-bridge A — zero motor current flows | Wired Pin 1 and Pin 15 cleanly to `GND` (Pin 8). |
| **3** | Single diode `D1` wired directly across reversible motor `M1` (`OUT1` $\leftrightarrow$ `OUT2`), no H-bridge flyback diodes | Forward-biases into a **dead short across `+12V` to `GND`** whenever motor reverses; inductive spikes damage `L298N` | Replaced single diode with a **4-diode `1N5822` Schottky clamp bridge** (`D1, D3` on `OUT1`, `D4, D5` on `OUT2` to `+12V` and `GND`). |
| **4** | Encoder `J2` Pin 3 and OLED `J4` Pin 2 powered from `+5V` | `5 V` encoder pulses (`CH_A`, `CH_B`) over-volt non-5V-tolerant `3.3 V` ESP32 GPIOs | Switched `#PWR011` (`J2`) and `#PWR019` (`J4`) to `+3.3V` (compatible with both 3.3 V-rated Hall encoders and 3.3 V SSD1306 OLEDs). |
| **5** | `/IN2` on ESP32 `GPIO12` (`MTDI` strapping pin), weak direct-GPIO `BZ1`, `WS2812B` `5 V` logic threshold margin | Pulling `GPIO12` high at boot sets ESP32 flash to `1.8 V` (boot failure); direct GPIO buzzer draw `< 3.3 mA`; `WS2812B` `V_IH = 0.7 * 5V = 3.5V > 3.3V` | Moved `/IN2` to `A1` Pin 21 (`GPIO32`), added `Q1` (`2N3904`) + flyback `D7` (`1N4148`) low-side buzzer driver, and added `D6` (`1N4148` drop to `~4.35 V`, `V_IH ~ 3.05 V`) + `C4` (`100 nF`) decoupling on `D2`. |

> **ERC Status**: `kicad-cli sch erc` improved from **14 Errors / 9 Warnings** down to **0 Errors / 3 Warnings** (the remaining 3 warnings are benign 3rd-party rescue library symbol cache notices for `U1`, `JP1`, and `U2`).

---

## 2. Finalized ESP32 DevKit V1 Pin Map (Synced With `firmware/include/board_pins.h`)

| Signal Net | Physical DOIT 30-Pin # | ESP32 GPIO | Direction / Peripheral |
|---|---:|---:|---|
| `/D0` (OLED SCLK) | Pin 6 | `GPIO16` | Output (SPI SCLK) |
| `/D1` (OLED MOSI) | Pin 7 | `GPIO17` | Output (SPI MOSI) |
| `/RES` (OLED Reset) | Pin 8 | `GPIO5` | Output |
| `/DC` (OLED Data/Cmd) | Pin 9 | `GPIO18` | Output |
| `/CS` (OLED Chip Select) | Pin 10 | `GPIO19` | Output |
| `/SDA` (`MPU6050`) | Pin 11 | `GPIO21` | Bidirectional (400 kHz I2C) |
| `/SCL` (`MPU6050`) | Pin 14 | `GPIO22` | Bidirectional (400 kHz I2C) |
| `/IMU_INT` | Pin 15 | `GPIO23` | Input (Data-ready optional) |
| `/IN2` (`L298N`) | Pin 21 | **`GPIO32`** | Output *(moved from `GPIO12`)* |
| `/Buzzer` (`Q1` Base) | Pin 22 | **`GPIO33`** | Output *(DOIT physical Pin 22 is `GPIO33`)* |
| `/CH_A` (Encoder A) | Pin 23 | `GPIO25` | Input (`PCNT` Unit 0 Ch A) |
| `/CH_B` (Encoder B) | Pin 24 | `GPIO26` | Input (`PCNT` Unit 0 Ch B) |
| `/LED_DATA` (`WS2812B`) | Pin 25 | `GPIO27` | Output (RMT TX) |
| `/PWM` (`L298N EnA`) | Pin 26 | `GPIO14` | Output (LEDC `20 kHz` PWM) |
| `/IN1` (`L298N`) | Pin 28 | `GPIO13` | Output |

> **Note on `MPU6050` Address Jumper `JP1`**: When `JP1` is soldered/closed (`ADO -> +3.3V`), `U3` responds at I2C address **`0x69`**; when open, **`0x68`**. The firmware in [hal_esp32.c](file:///home/thanotos/BalanceCube/firmware/src/hal_esp32.c) probes both automatically at boot.

---

## 3. Optional PCB-Stage Driver Upgrade Note
Before routing [Balance_cube.kicad_pcb](file:///home/thanotos/BalanceCube/Schematics/Balance_cube.kicad_pcb), consider swapping `U2` (`L298N` + 4x `1N5822` diodes) for a single compact MOSFET driver IC such as **`DRV8874`** or **`TB67H450FNG`** — same 3-pin MCU interface (`PWM`, `IN1`, `IN2`), no external clamp diodes required, almost zero heat dissipation, and `+1.8 V` more motor voltage headroom (~`+3 deg` wider catch window).
