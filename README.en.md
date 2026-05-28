# camera_speed_test

**Languages:** **English** (this page) · [Русский](README.md)

---

Firmware for **Arduino Nano** (PlatformIO) — a device for measuring **glass-to-glass** latency in the video path “camera → transport (analog / Ethernet / Wi‑Fi / RF) → operator monitor”. Stimulus is a bicolor LED switching between **red** and **blue** (no full blackout, so camera auto-exposure does not hunt). The photodetector is pressed against the monitor screen in a light-tight hood. On-device display — **SSD1306 128×64** over I2C. Wiring — [Pinout](#pinout) section below.

## Pinout

OLED **SSD1306 128×64** (I2C), button, bicolor LED, photodetector on **A1**. Control and logs are also available via **USB / Serial** (`115200 8N1`) — the device is fully operational without OLED and without a button.

### SSD1306 128×64 (I2C)

Typical pin labels: **GND, VCC, SCL, SDA** (order may differ — check silkscreen).

| Module pin | Arduino Nano V3       | Notes                                                                                                      |
| ---------- | --------------------- | ---------------------------------------------------------------------------------------------------------- |
| **GND**    | **GND**               | Common ground                                                                                              |
| **VCC**    | **5 V** *or* **3V3**  | Some modules with a regulator — **5 V**; bare 3.3 V chip — **3V3** only (see board marking)                |
| **SCL**    | **A5**                | **SCL** on `Wire` bus                                                                                      |
| **SDA**    | **A4**                | **SDA** on `Wire` bus                                                                                      |

Default I2C address in code: **`0x3C`** (`cfg::kOledI2cAddress7bit` in `include/Config.h`). If the display does not respond — try **`0x3D`** or check the **ADDR** jumper on the board.

**RST** on the module, if present, is often left unconnected — `Config.h` sets **`kOledResetPin = -1`** (software reset over I2C).

**OLED is optional.** With no display, set **`kOledEnabled = false`** in `include/Config.h` — firmware will not touch I2C (A4/A5), no hang on a floating bus. Control and logs — Serial only (see “Control and monitoring via Serial”). When you add the module — **`kOledEnabled = true`** and reflash.

### Button → **D2**

| Button contact | Nano    |
| -------------- | ------- |
| One leg        | **D2**  |
| Other leg      | **GND** |

`INPUT_PULLUP`: idle **HIGH**, pressed **LOW**.

**Button is optional.** All gestures are duplicated by Serial commands `m` / `c` / `r` / `h`.

### Bicolor LED → **D4**, **D5**

Assumes a **two-pin bicolor** — two LEDs (red/blue) anti-parallel in one package. Current one way — red on, the other way — blue on, no current — both off.

| LED pin     | Connection |
| ----------- | ---------- |
| Pin **1**   | **D4**     |
| Pin **2**   | **D5**     |

**Current-limiting resistor** (e.g. **330 Ω**) in **one** of the two wires is enough: current in either direction passes through the same resistor. Two resistors are not required.

| **D4**                               | **D5** | Result                    |
| ------------------------------------ | ------ | ------------------------- |
| `HIGH`                               | `LOW`  | One color (Red in firmware) |
| `LOW`                                | `HIGH` | Other color (Blue in firmware) |
| Same (both `LOW` or both `HIGH`)     |        | LED off                   |

Color mapping in `BicolorLed::set`:

- `LedColor::Red`  → D4 = HIGH, D5 = LOW
- `LedColor::Blue` → D4 = LOW,  D5 = HIGH

If colors look swapped — swap D4/D5 wires (or swap the two `case` branches in `BicolorLed::set`). Measurement correctness is unaffected: `dirSign` is computed automatically during calibration.

### Photodetector → **A1**

Either of two types works. Polarity (dark → high or low ADC code) does not matter: firmware computes `cR`/`cB` and determines which side of the threshold to expect for each transition.

#### Option A — NPN phototransistor

**Collector** pull-up **10 kΩ** to **5 V**, emitter to **GND**. Signal from collector → **A1**.

- Dark → collector pulled to 5 V → high ADC code (≈ 1023)
- Brighter on sensor → collector pulled down → low ADC code

#### Option B — LDR (photoresistor)

LDR in a divider with **10 kΩ** pull-up. Two wirings:

- LDR between **A1** and **5 V**, **10 kΩ** from **A1** to **GND** → bright = **high** code.
- LDR between **A1** and **GND**, **10 kΩ** from **A1** to **5 V** → bright = **low** code.

#### Mechanical

The photodetector must be **pressed against the monitor screen in a light-tight hood** (suction cup, rubber sleeve, heat-shrink, etc.). Stray light collapses both levels (R and B converge) and is caught automatically as `dRB low`.

### USB / Serial → built-in Nano USB bridge (CH340 / FT232)

Connect with a **USB cable** to a PC. On the board this channel uses two digital pins:

| Signal | Nano pin | Purpose            |
| ------ | -------- | ------------------ |
| TX     | **D1**   | UART TX (USB ↔ PC) |
| RX     | **D0**   | UART RX (USB ↔ PC) |

- Baud: **115200 8N1**.
- Commands (one character, case-insensitive): `m` — menu, `c` — calibrate, `r` — measure, `h` — help.
- All screens are mirrored as lines with prefixes `[MENU]`, `[CAL]`, `[CAL_RESULT]`, `[WARMUP]`, `[LAT]`, `[RESULT]`, `[ABORT]`.

> `D0` and `D1` are on the Nano header but **do not use** them as GPIO — that breaks the Serial console.

### Pin summary

| Pin              | Function              | Can omit?                   |
| ---------------- | --------------------- | --------------------------- |
| **A4**           | OLED SDA (I2C)        | yes (Serial only)           |
| **A5**           | OLED SCL (I2C)        | yes                         |
| **D2**           | Button → GND          | yes (Serial only)           |
| **D4**           | Bicolor LED (pin 1)   | **no** — stimulus           |
| **D5**           | Bicolor LED (pin 2)   | **no** — stimulus           |
| **A1**           | Photodetector         | **no** — sensor             |
| **D0**, **D1**   | USB Serial (RX/TX)    | **no** — needed for console |
| **5 V**, **GND** | Peripheral power      | —                           |

#### Minimum working setup

Only what is required to measure:

- **Bicolor LED** on **D4/D5** (with one current-limiting resistor)
- **Photodetector** on **A1** (with 10 kΩ pull-up)
- **USB cable** to PC (power + Serial)

OLED and button are optional.

### Free pins for future modules

Free (after the above): **D3**, **D6–D13**, **A0**, **A2–A3**, **A6–A7**.

- **D11**, **D12**, **D13** — SPI (`MOSI / MISO / SCK`). Available as GPIO until SPI peripherals are added.
- **A6**, **A7** on classic Nano — **ADC only**, no digital mode (no port register).

Enough room for extensions: extra photodetector, logic-analyzer marker on color change (e.g. **D3**), external LED driver, brightness potentiometer.

## Build and flash

Default — **Optiboot** new bootloader (`nanoatmega328new`, 115200):

```bash
pio run -t upload
```

Old bootloader (rare):

```bash
pio run -e nanoatmega328 -t upload
```

Port in [`platformio.ini`](platformio.ini): uncomment `upload_port = /dev/ttyUSB1` (or your `ttyUSB0` / `ttyACM0`).

### Error `avrdude: stk500_getsync()`

| Cause | Fix |
|-------|-----|
| Wrong bootloader | Try `pio run -t upload` (new) first. Still fails → `pio run -e nanoatmega328 -t upload` (old, 57600). |
| Port busy | Close Serial Monitor, `screen`, Arduino IDE, `minicom` on that tty. |
| No tty permissions | `sudo usermod -aG dialout $USER`, re-login; or once `sudo pio run -t upload`. |
| Wrong cable/port | **Data** USB only; `dmesg` should show `ch341` → `ttyUSB*`. |
| Reset needed | During “Uploading…”, double-tap **RESET** on Nano (or hold RESET, release when upload starts). |

After success: `pio device monitor` (115200, as in firmware).

## Stack

- **MCU:** ATmega328P (Arduino Nano)
- **Display:** SSD1306 128×64 (I2C, `A4`/`A5`, default address `0x3C` — see `cfg::kOledI2cAddress7bit` in [`include/Config.h`](include/Config.h)). Default font 5×8 → ≈21 chars × 8 lines. **Optional** — full control via Serial (below).
- **Serial console:** `115200 8N1`, single-character commands (`m` / `c` / `r` / `h`), all screens mirrored with `[MENU]`, `[CAL]`, `[RESULT]`, etc. Details — “Control and monitoring via Serial”.
- **All runtime parameters:** [`include/Config.h`](include/Config.h)

## Control

| Gesture | Action from main menu |
|---------|------------------------|
| Short single tap | Exit mode / refresh main menu |
| Double short tap | Explicit R / B calibration (2 phases × `kCalibPhaseDurationMs`) |
| Long hold (≥ 3 s) | Auto warmup and latency measurement (`kLatencySessionMs`) |

From any screen, a short single tap returns to the main menu.

### Control and monitoring via Serial

The device is fully operational **without OLED** — all screens are mirrored on `Serial` (`115200 8N1`), and the button is fully replaced by single-letter commands. You can leave the display disconnected.

Commands (case-insensitive, one character, extra spaces/`\r`/`\n` ignored):

| Char | Gesture equivalent | Action |
|------|-------------------|--------|
| `m` | Single | Return to main menu / exit any mode |
| `c` | DoubleShort | Start explicit R/B calibration |
| `r` | LongHold | Auto warmup and 15 s measurement |
| `h` or `?` | — | Print help |

**Button** gestures and **Serial** commands work in parallel; whichever arrives first in a given `loop()` tick is handled.

After boot, Serial prints a banner and help:

```
=== KRAN glass2glass: boot ===

--- KRAN glass2glass: serial console ---
commands (one char, case-insensitive):
  m  - menu / back / exit any screen
  c  - CALIBRATION (R/B, 2 phases x 5s)
  r  - RUN measurement (warmup + 15s R<->B)
  h  - this help
---
```

Each screen is mirrored as one line with a bracketed prefix:

```
[MENU]  m=back  c=cal(2ph x 5s)  r=run(22s, max~500ms)  h=help
[CAL] phase=1/2 RED  avg=812 mn=810 mx=814 sprd=4 n=320
[CAL] phase=2/2 BLUE avg=210 mn=208 mx=213 sprd=5 n=320

[CAL_RESULT] R=812 +/-4  B=210 +/-5  dRB=-602  status=OK
[WARMUP] phase=1/2  LED=RED
[WARMUP] phase=2/2  LED=BLUE
[LAT] t=0.3/22.5  n=0 lost=0  min=0.0 avg=0.0 max=0.0  RB=0.0 BR=0.0
[LAT] t=1.0/22.5  n=1 lost=0  min=42.3 avg=42.3 max=42.3  RB=42.3 BR=0.0
...
[LAT] t=22.5/22.5 n=28 lost=2  min=12.4 avg=42.7 max=78.1  RB=41.5 BR=43.9

[RESULT] GLASS->GLASS, ms:
  min  = 12.40
  avg  = 42.70
  max  = 78.10
  RB av= 41.50
  BR av= 43.10
  n    = 28
  lost = 2
```

Prefixes `[MENU]`, `[CAL]`, `[CAL_RESULT]`, `[WARMUP]`, `[LAT]`, `[RESULT]`, `[ABORT]` are easy to filter in logs (`grep`/`Select-String`) or parse for test reports.

On measurement abort:

```
[ABORT] reason=dRB low  R=512  B=528  dRB=16
```

User commands are echoed with `>>`:

```
>> [cmd] CALIBRATION
```

So the log shows both who initiated an action and the outcome.

## Measurement principle

1. **2-phase scanner** (shared by calibration and warmup):
   - **RED** — LED red, level `cR` measured.
   - **BLUE** — LED blue, level `cB` measured.

   Background is **not** measured as a separate phase: the LED never turns off during a session; background influence is caught by the two feasibility checks below.

2. **Threshold calculation** between color levels (adaptive to signal strength):
   - `mid = (cR + cB) / 2`
   - `H = clamp(|cR − cB| / kAdaptiveHysteresisSpreadDiv, kAdaptiveHysteresisMin … Max)` — weak beam on sensor (dRB≈12…30) pulls threshold toward `mid`
   - `tLow = mid − H`, `tHigh = mid + H`
   - `dirSign = sign(cB − cR)` — which side of the threshold to expect each transition.
   - After cal/warmup on Serial: `[CAL_THRESH] mid=… H=… tLo=… tHi=…`

3. **Feasibility check** before measurement:
   - `|cR − cB| ≥ kFeasibleMinSpreadAdc` (default **10**) — else `dRB low`
   - `|cR − cB| ≥ max(sprdR, sprdB) + kFeasibleSpreadAboveNoise` — color difference must exceed phase noise
   - `cR` and `cB` not ADC-saturated — else `ADC clip` (adjust detector pull-up / monitor brightness).
   - On failure — **ABORT** screen with reason and levels.

4. **R↔B measurement** (`kLatencySessionMs`): LED toggles red ↔ blue every `kLatencyBlinkHalfPeriodMs`. On color-change command, `t0 = micros()`; then wait for ADC to cross `mid` in the expected direction (with hysteresis). First crossing — record `Δt`.

5. **Bidirectional statistics**: **R→B** and **B→R** delays are tracked separately (Bayer/ISP/panel asymmetry).

## Measurement limits and tuning

Latency is capped by **stimulus half-period** and **sensor timeout**. Derived from one constant `cfg::kLatencyMaxExpectedMs` (ms):

```
kLatencyBlinkHalfPeriodMs = 1.5 × kLatencyMaxExpectedMs   // stimulus half-period
kLatencySensorTimeoutUs   = 1.2 × kLatencyMaxExpectedMs   // edge timeout
kLatencySessionMs         = max(15 s, 30 × half-period)    // session length
```

Rule: **`kLatencyMaxExpectedMs` must be ≥ real path latency**, otherwise samples are lost (delay > timeout) or wrong edges are captured (delay > half-period) → invalid results.

### Presets

Change one line in `include/Config.h`:

| Scenario | `kLatencyMaxExpectedMs` | Half-period | Timeout | Session | Note |
|----------|-------------------------|-------------|---------|---------|------|
| HDMI / low-latency analog | `150` | 225 ms | 180 ms | 15 s | ~66 toggles |
| OpenIPC RTSP UDP local | `300` | 450 ms | 360 ms | 15 s | ~33 toggles |
| **IP / RTSP / Wi‑Fi (default)** | `500` | 750 ms | 600 ms | **22.5 s** | ~30 toggles |
| Wi‑Fi with heavy buffering | `1000` | 1500 ms | 1200 ms | **45 s** | ~30 toggles |
| Extreme (bad WAN, many buffers) | `2000` | 3000 ms | 2400 ms | **90 s** | ~30 toggles |

### If latency exceeds `kLatencyMaxExpectedMs`

| Real delay | What you see |
|------------|--------------|
| ≤ `1.2 × max` | Normal, samples captured |
| between `1.2 × max` and `1.5 × max` | Most or all samples go to **`lost`**, small or zero `n` in final |
| > `1.5 × max` (i.e. > half-period) | **Aliasing**: next LED toggle before current edge arrives; false low delays and/or mass `lost`. Do not trust such results |

If the final report shows many `lost` or suspiciously low values on a slow path — **raise** `kLatencyMaxExpectedMs` and reflash.

### Manual half-period / timeout only

Possible but little benefit — formulas are simple and consistent. If tuning manually: keep `timeout < half-period < ½ average interval between path edges`.

## On-screen display

All screens fit 21 characters × 8 lines (SSD1306 128×64, default font).

### Abbreviation glossary

| Short | Full | Meaning |
|-------|------|---------|
| `R` | Red level | Average ADC when LED is red (0…1023) |
| `B` | Blue level | Average ADC when LED is blue (0…1023) |
| `sprd` / `+/-` | Spread | `max − min` ADC in a phase — rough noise/stability estimate |
| `dRB` | Delta R↔B | `cB − cR` in ADC codes. Sign sets `dirSign`. Larger \|dRB\| — more reliable edge detection |
| `mn` / `mx` | Min / Max | Min and max ADC in current phase |
| `n` | samples / count | Samples collected so far |
| `lost` | lost samples | LED toggles that ended in **timeout** (edge not seen in ~200 ms window) |
| `RB` | Red→Blue avg, ms | Average red→blue delay, milliseconds |
| `BR` | Blue→Red avg, ms | Average blue→red delay, milliseconds |
| `min/avg/max` | in final/live | Min, mean, max delay over **all** samples both directions, ms |

> One ADC code ≈ 5.0 V / 1024 ≈ **4.9 mV** (reference — `cfg::kAdcVrefVolts`).

---

### 1. Main menu

```
KRAN  glass2glass
tap   = back
2tap  = CAL R/B
hold  = MEASURE

cal:  2ph x 5s
meas: 22s R<->B
max ~500ms
```

Times and max delay **come from `Config.h`** — changing `kLatencyMaxExpectedMs` updates OLED and Serial automatically.

| Line | Meaning |
|------|---------|
| `KRAN  glass2glass` | Device title |
| `tap   = back` | Short single tap → menu / exit mode |
| `2tap  = CAL R/B` | Two short taps → **explicit calibration** (2 × `kCalibPhaseDurationMs`) |
| `hold  = MEASURE` | Hold ≥ 3 s → auto warmup and **measurement** for `kLatencySessionMs` |
| `cal:  2ph x Ns` | Actual calibration phase length |
| `meas: Ns R<->B` | Actual session length and stimulus mode |
| `max ~Nms` | Upper measurable delay (`kLatencyMaxExpectedMs`) |

---

### 2. Calibration phase screen

```
CAL 1/2  RED
adc   812
mn 810  mx 814
sprd  4
n     320

tap = menu
```

| Field | Example | Meaning |
|-------|---------|---------|
| `CAL 1/2  RED` | 1/2, RED | Phase number and label: 1=RED, 2=BLUE |
| `adc 812` | 812 | Running **average** ADC this phase |
| `mn 810  mx 814` | 810 / 814 | Min/max ADC this phase (difference = flicker/noise) |
| `sprd 4` | 4 | `mx − mn`. Small spread (< 10) — stable light. Large (> 50) — flicker / bad contact / stray light |
| `n 320` | 320 | Samples in this phase (poll every `kCalibSamplePeriodMs = 15 ms`) |
| `tap = menu` | — | Short tap aborts calibration |

Expected: `RED` → one stable ADC level, `BLUE` → clearly different. Nearly equal levels → next screen `FAIL: dRB low`.

---

### 3. Calibration result

```
CAL  RESULT

R    812 +/-4
B    210 +/-5
dRB  -602

status: OK
tap = menu
```

| Field | Example | Meaning |
|-------|---------|---------|
| `R 812 +/-4` | 812, spread 4 | Level with red. Should not be near 0 or 1023 (saturation) |
| `B 210 +/-5` | 210, spread 5 | Level with blue |
| `dRB -602` | -602 | `cB − cR`. Here negative — red brighter than blue (`dirSign = -1`). \|602\| — excellent spread |
| `status: OK` | — | Spread ≥ 10, dRB above phase noise, no ADC clip |
| or `FAIL: <reason>` | `dRB low` / `ADC clip` | Failure reason (see ABORT table) |

---

### 4. Auto warmup before measurement

```
WARMUP
phase 1/2
LED   RED

checking R/B
levels ...

tap = menu
```

| Field | Example | Meaning |
|-------|---------|---------|
| `WARMUP` | — | Quick auto warmup before each run (~1 s total: 2 × `kWarmupPhaseDurationMs = 500 ms`) |
| `phase 1/2` | 1 | 2-phase scanner phase |
| `LED   RED` | RED | Current LED color (RED / BLUE) |
| `checking R/B levels …` | — | Measuring levels and feasibility |
| `tap = menu` | — | Abort warmup |

After warmup → **measurement** or **ABORT**.

---

### 5. ABORT — cannot measure

```
ABORT  no measure
reason: dRB low


R    512
B    528
dRB  16

tap = menu
```

| Field | Example | Meaning |
|-------|---------|---------|
| `ABORT  no measure` | — | Refused to start measurement after warmup |
| `reason: <code>` | `dRB low` | Reason (table below) |
| `R / B / dRB` | ADC values | Diagnostics |
| `tap = menu` | — | Back to menu |

Reasons:

| reason | What happened | What to do |
|--------|---------------|------------|
| `dRB low` | `\|cR − cB\| < 10` or difference not above phase noise | Improve light on A1 (hood, focus); for dRB 12…18 check `[CAL_THRESH]` — often `H=2…3` |
| `ADC clip` | `cR` or `cB` ≤ 4 or ≥ 1019 — ADC saturated | Reduce LED current or adjust pull-up; lower monitor brightness |
| `no data` | scanner got no samples | Should not happen; check ADC polling |

---

### 6. Live measurement screen

```
LAT 4.3/22.5
n 28  lost 1
min  12.4 ms
avg  42.7 ms
max  78.1 ms

RB 41.5  BR 43.9
tap = exit
```

| Field | Example | Meaning |
|-------|---------|---------|
| `LAT 4.3/22.5` | 4.3 / 22.5 | Elapsed s / total session s (`kLatencySessionMs / 1000`) |
| `n 28` | 28 | Valid samples (both directions) |
| `lost 1` | 1 | Lost samples (sensor timeout — edge not seen in ~200 ms) |
| `min  12.4 ms` | 12.4 ms | **Minimum** delay all samples |
| `avg  42.7 ms` | 42.7 ms | **Mean** all samples |
| `max  78.1 ms` | 78.1 ms | **Maximum** (often limited by camera frame period and monitor refresh) |
| `RB 41.5  BR 43.9` | 41.5 / 43.9 | Mean delay `Red→Blue` and `Blue→Red` |
| `tap = exit` | — | Short tap aborts measurement |

OLED refresh ~4 Hz (`kLatencyUiPeriodMs = 250 ms`), **only between edges** (while waiting for threshold crossing after LED change, display and Serial are not updated). `[LAT]` on Serial — once per second (`kLatencySerialPeriodMs`) so `loop()` is not blocked at 115200. While waiting for an edge, ADC is polled up to `kLatencySensorPollsPerLoop` times per `loop()`. If `lost` grows fast — path dropped stream or camera/monitor cannot keep up with stimulus half-period (750 ms at `max=500`).

---

### 7. Final `GLASS→GLASS` screen

```
GLASS->GLASS  ms
min  12.40
avg  42.70
max  78.10
RB   41.50
BR   43.10
n=240  lost=3
tap = menu
```

| Field | Example | Meaning |
|-------|---------|---------|
| `GLASS->GLASS  ms` | — | All values below in **milliseconds** |
| `min 12.40` | 12.40 ms | Session minimum — “best case”, often when LED toggle aligned with new camera frame |
| `avg 42.70` | 42.70 ms | Mean — main number for reports |
| `max 78.10` | 78.10 ms | Maximum — “worst case”, jitter indicator. If `max − min` > 2/FPS — normal frame quantization. Much larger — decoder buffering, packet loss, vsync |
| `RB 41.50` | 41.50 ms | Mean `Red→Blue` |
| `BR 43.10` | 43.10 ms | Mean `Blue→Red`. 1–10 ms difference — normal (Bayer/ISP/panel asymmetry) |
| `n=240` | 240 | Valid samples in statistics. Expected ≈ `kLatencySessionMs / kLatencyBlinkHalfPeriodMs`. Default (`max=500 ms`) ~30 |
| `lost=3` | 3 | Toggles that timed out. `lost / (n + lost) < 5 %` — good |
| `tap = menu` | — | Back to menu. Result not saved to EEPROM — RAM only until reboot/new run |

---

### Quick “how to read results”

| Observation | Interpretation |
|-------------|----------------|
| `avg ≈ min ≈ max` | Stable path, little jitter (rare on IP/Wi‑Fi) |
| `max − min` ≈ 1–2 / camera FPS | Normal frame quantization |
| `max ≫ avg` (many times) | Decoder buffering / Wi‑Fi retries |
| `RB` ≠ `BR` strongly (> 15 ms) | Strong color/panel asymmetry; retry with swapped colors or another monitor |
| `lost` > 10 % | Half-period too short for path, or link loss. Increase `kLatencyBlinkHalfPeriodMs` |

## Calibration storage

**RAM only** (`app::CalibData`). Lost on reboot — keeps firmware simple and thresholds match current setup. Before each measurement, **short auto warmup** (2 × `cfg::kWarmupPhaseDurationMs`) recomputes `cR / cB / mid` and checks feasibility.

## What is and is not measured

The device measures **full end-to-end** video path latency: camera optics → sensor → ISP → encoder → transport → decoder → compositor → monitor panel → pixel → photodiode. Path can be **any** — analog, IP, wireless.

**Included**: LCD panel response time. **Excluded**: MCU command time only (tens of µs) — negligible vs typical tens of ms path delay.

## Repeatable measurement procedure

1. Warm up camera and monitor at least 10 minutes.
2. On camera, lock exposure/gain/white balance (e.g. OpenIPC `majestic.yaml` — `exposure: manual`), disable “dynamic” processing (HDR, NR if possible).
3. On monitor, lock brightness, disable “dynamic contrast” and “overdrive” (or fix profile).
4. Press photodetector to monitor in hood (no stray light); place LED in sight hood before camera (no glare).
5. `2tap` — explicit calibration, confirm `status: OK`.
6. Long hold — auto warmup and measure. Repeat 5–10 times.
7. Log: `min / avg / max`, `RB`, `BR`, `n`, `lost`, path config (codec, resolution, FPS, bitrate, transport, monitor refresh).

## Source layout

| Path | Purpose |
|------|---------|
| [`include/Config.h`](include/Config.h) | All constants (pins, periods, feasibility thresholds) |
| [`include/Application.h`](include/Application.h) | Phases, `CalibData`, orchestration |
| [`src/Application.cpp`](src/Application.cpp) | 4-phase scanner, R↔B measurement, gesture handling |
| [`include/DisplayUi.h`](include/DisplayUi.h), [`src/DisplayUi.cpp`](src/DisplayUi.cpp) | All 21×8 screens |
| [`include/BicolorLed.h`](include/BicolorLed.h), [`src/BicolorLed.cpp`](src/BicolorLed.cpp) | Bicolor LED (Off/Red/Blue) |
| [`include/PhotoSensor.h`](include/PhotoSensor.h), [`src/PhotoSensor.cpp`](src/PhotoSensor.cpp) | Photodetector ADC and statistics |
| [`include/ButtonInput.h`](include/ButtonInput.h), [`src/ButtonInput.cpp`](src/ButtonInput.cpp) | Button gesture decoder |
| [`include/SerialInput.h`](include/SerialInput.h), [`src/SerialInput.cpp`](src/SerialInput.cpp) | Serial commands (emulate button) |

## Tests and utilities

See [test/README.en.md](test/README.en.md) ([Русский](test/README.md)) — PlatformIO unit tests and `phototransistor_screen_test.py` on-screen stimulus tool.
