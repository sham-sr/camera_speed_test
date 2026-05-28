# Tests and helper utilities

**Languages:** **English** (this page) · [Русский](README.md)

---

## PlatformIO unit tests

The `test/` directory is for [PlatformIO unit tests](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html).  
Firmware, calibration, and latency measurement: [README.md](../README.md) ([English](../README.en.md)) in the repository root.

---

## On-screen stimulus (`phototransistor_screen_test.py`)

Python utility in the project root: fullscreen red/blue stimulus on the monitor to verify the photodetector on screen. Timings and scenarios match the Arduino firmware (`include/Config.h`, `SerialInput.cpp`).

### Installation

```bash
python -m pip install -r requirements-screen-test.txt
```

Requires **Python 3** and **tkinter** (usually included with the standard Windows Python build). For Nano communication — **pyserial**.

### Run

```bash
python phototransistor_screen_test.py
```

Options:

- `--port COM7` or `--port /dev/ttyACM0` — pre-select port (Windows / Linux).
- `--windowed` — windowed mode instead of fullscreen.

### Controls

Same commands as the firmware Serial Monitor (115200):

| Key | Serial | Action |
|-----|--------|--------|
| **m** | `m` | Menu, cancel, screen off |
| **c** | `c` | Calibration: 5 s red + 5 s blue |
| **r** | `r` | Warmup 2.5+2.5 s, then R↔B measurement |
| **h** / **?** | `h` | Help |
| **Esc** | — | Quit |

When a COM port is selected, the key is sent to the device and **in parallel** the local on-screen stimulus runs.

The **Serial** block in the window mirrors firmware lines (`[CAL]`, `[CAL_RESULT]`, `[ABORT]`, `[LAT]`, `[RESULT]`, etc.). If the device reports **`status=FAIL`** (e.g. `dRB low`) or **`[ABORT]`**, the local stimulus **stops** — measurement on screen does not continue, same as on the Arduino.

### Ports (Windows / Ubuntu)

Refresh the port list with **“Refresh”**. **“(none — screen only)”** — run without Arduino.

On Linux, if access is denied: `sudo usermod -aG dialout $USER` and log in again.

### Screen delay

Slider **“Delay (measurement only)”**: **5 ms** steps, ±500 ms range. Shifts color changes **only in the measurement phase** (emulates picture lag relative to the LED). Warmup and calibration from Arduino run **without offset**.

### Timings (defaults in `Config.h`)

| Mode | Duration |
|------|----------|
| Calibration | 2× 5000 ms (R, B) |
| Warmup | 2× 500 ms (R, B) |
| Measurement | half-period 750 ms, session 22.5 s |

When you change `kLatencyMaxExpectedMs` in firmware, update the same constants at the top of `phototransistor_screen_test.py`.
