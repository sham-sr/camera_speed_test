#!/usr/bin/env python3
"""
Стимул на мониторе для теста фототранзистора — поведение и тайминги как в прошивке Arduino
(include/Config.h, Application.cpp, SerialInput.cpp).

Клавиши (как serial, без учёта регистра):
  m — меню (выход / отмена)
  c — калибровка (2×5 с R/B, робастные пороги на устройстве)
  r — прогрев (2×2.5 с R/B) + замер (R↔B, полупериод и длина сессии из Config.h)
  h / ? — подсказка

При выбранном COM-порте те же символы отправляются на устройство (115200).
Читается ответ прошивки ([CAL_RESULT], [ABORT], …): лог в окне; при FAIL/ABORT экран гаснет.
Ползунок «задержка экрана»: шаг 5 мс, только на фазе замера (прогрев/калибровка без сдвига).
"""

from __future__ import annotations

import argparse
import re
import sys
import time
import tkinter as tk
from tkinter import font as tkfont, ttk
from typing import Callable

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None  # type: ignore[assignment]
    list_ports = None  # type: ignore[assignment]

# --- cfg:: из include/Config.h -----------------------------------------------
LATENCY_MAX_EXPECTED_MS = 500
BLINK_HALF_PERIOD_MS = (LATENCY_MAX_EXPECTED_MS * 3) // 2
SESSION_MS = max(15_000, BLINK_HALF_PERIOD_MS * 30)

CALIB_PHASE_DURATION_MS = 5_000
WARMUP_PHASE_DURATION_MS = 2_500
SCAN_PHASE_COUNT = 2

SERIAL_BAUD = 115200
DELAY_STEP_MS = 5
DELAY_SLIDER_MIN = -100
DELAY_SLIDER_MAX = 100

RED = "#e00000"
BLUE = "#0000e0"
OFF_SCREEN = "#121212"
HUD_BG = "#101010"
HUD_FG = "#f0f0f0"
HUD_ACCENT = "#9cf"

NO_PORT_LABEL = "(нет — только экран)"

# Строки прошивки (DisplayUi.cpp / Application.cpp), которые показываем в логе.
DEVICE_LOG_MARKERS = (
  "=== KRAN",
  "--- KRAN",
  "[MENU]",
  "[CAL]",
  "[CAL_RESULT]",
  "[CAL_THRESH]",
  "[WARMUP]",
  "[ABORT]",
  "[LAT]",
  "[RESULT]",
  ">> [cmd]",
  "?? unknown",
)

RE_CAL_FAIL = re.compile(r"\[CAL_RESULT\].*status=FAIL:\s*(.+)", re.IGNORECASE)
RE_ABORT = re.compile(r"\[ABORT\]\s*reason=(\S+)", re.IGNORECASE)
RE_CAL_OK = re.compile(r"\[CAL_RESULT\].*status=OK", re.IGNORECASE)
RE_RESULT = re.compile(r"\[RESULT\]", re.IGNORECASE)

SERIAL_LOG_LINES = 80


def list_serial_ports() -> list[str]:
    if list_ports is None:
        return []
    return sorted(p.device for p in list_ports.comports())


class SerialBridge:
  """Отправка команд и приём строк с Arduino (115200)."""

  def __init__(self) -> None:
    self._ser: serial.Serial | None = None
    self._port: str | None = None
    self._rx_buffer = ""

  @property
  def port(self) -> str | None:
    return self._port

  @property
  def connected(self) -> bool:
    return self._ser is not None and self._ser.is_open

  def open(self, port: str | None) -> None:
    self.close()
    if not port or port == NO_PORT_LABEL:
      self._port = None
      return
    if serial is None:
      raise RuntimeError("Установите pyserial: pip install -r requirements-screen-test.txt")
    self._rx_buffer = ""
    self._ser = serial.Serial(port, SERIAL_BAUD, timeout=0)
    self._port = port

  def close(self) -> None:
    if self._ser is not None:
      try:
        self._ser.close()
      except OSError:
        pass
      self._ser = None
    self._port = None
    self._rx_buffer = ""

  def poll_lines(self) -> list[str]:
    """Прочитать полные строки из буфера (неблокирующе)."""
    if not self.connected:
      return []
    assert self._ser is not None
    try:
      waiting = self._ser.in_waiting
      if waiting:
        chunk = self._ser.read(waiting)
        self._rx_buffer += chunk.decode("utf-8", errors="replace")
    except OSError:
      return []
    lines: list[str] = []
    while "\n" in self._rx_buffer:
      raw, self._rx_buffer = self._rx_buffer.split("\n", 1)
      line = raw.strip("\r").strip()
      if line:
        lines.append(line)
    return lines

  def send_command(self, ch: str) -> bool:
    if not self.connected or len(ch) != 1:
      return False
    assert self._ser is not None
    self._ser.write(ch.encode("ascii"))
    self._ser.flush()
    return True


class StimulusApp:
  def __init__(self, root: tk.Tk, initial_port: str | None) -> None:
    self.root = root
    self.bridge = SerialBridge()
    self._pending: list[str] = []
    self._timeline_start = 0.0
    self._mode = "idle"
    self._tick_after: str | None = None
    self._serial_poll_after: str | None = None
    self._device_abort = False

    root.title("Phototransistor screen test")
    root.configure(bg=HUD_BG)

    self._build_ui()
    self._refresh_ports(initial_port)
    self.enter_menu("Готово. m — меню, c — калибровка, r — замер")

  # --- UI -------------------------------------------------------------------

  def _build_ui(self) -> None:
    top = tk.Frame(self.root, bg=HUD_BG)
    top.pack(side=tk.TOP, fill=tk.X, padx=8, pady=6)

    self._font = tkfont.nametofont("TkDefaultFont")
    self._font.configure(size=10)

    row1 = tk.Frame(top, bg=HUD_BG)
    row1.pack(fill=tk.X)

    tk.Label(row1, text="COM:", bg=HUD_BG, fg=HUD_FG, font=self._font).pack(side=tk.LEFT)
    self.port_var = tk.StringVar()
    self.port_combo = ttk.Combobox(
      row1, textvariable=self.port_var, width=28, state="readonly", font=self._font
    )
    self.port_combo.pack(side=tk.LEFT, padx=(4, 4))
    self.port_combo.bind("<<ComboboxSelected>>", self._on_port_selected)

    ttk.Button(row1, text="Обновить", command=self._refresh_ports_click).pack(side=tk.LEFT, padx=2)
    self.serial_led = tk.Label(row1, text="●", bg=HUD_BG, fg="#666", font=self._font)
    self.serial_led.pack(side=tk.LEFT, padx=6)
    self.serial_label = tk.Label(row1, text="нет связи", bg=HUD_BG, fg="#888", font=self._font)
    self.serial_label.pack(side=tk.LEFT)

    row2 = tk.Frame(top, bg=HUD_BG)
    row2.pack(fill=tk.X, pady=(6, 0))

    tk.Label(row2, text="Задержка (только замер):", bg=HUD_BG, fg=HUD_FG, font=self._font).pack(
      side=tk.LEFT
    )
    self.delay_var = tk.IntVar(value=0)
    self.delay_scale = tk.Scale(
      row2,
      from_=DELAY_SLIDER_MIN,
      to=DELAY_SLIDER_MAX,
      orient=tk.HORIZONTAL,
      variable=self.delay_var,
      resolution=1,
      length=320,
      bg=HUD_BG,
      fg=HUD_FG,
      troughcolor="#333",
      highlightthickness=0,
      command=self._on_delay_changed,
    )
    self.delay_scale.pack(side=tk.LEFT, padx=8)
    self.delay_label = tk.Label(row2, text="0 мс", bg=HUD_BG, fg=HUD_ACCENT, font=self._font)
    self.delay_label.pack(side=tk.LEFT)

    help_text = (
      "m — меню | c — калибровка (5+5 с) | r — прогрев (2.5+2.5 с) + замер "
      f"({SESSION_MS // 1000} с, полупериод {BLINK_HALF_PERIOD_MS} мс) | h — помощь | Esc — выход"
    )
    self.help_lbl = tk.Label(
      top, text=help_text, bg=HUD_BG, fg=HUD_FG, font=self._font, anchor="w", justify=tk.LEFT
    )
    self.help_lbl.pack(fill=tk.X, pady=(6, 0))

    self.status = tk.Label(top, text="", bg=HUD_BG, fg=HUD_ACCENT, font=self._font, anchor="w")
    self.status.pack(fill=tk.X, pady=(4, 0))

    log_frame = tk.Frame(top, bg=HUD_BG)
    log_frame.pack(fill=tk.X, pady=(4, 0))
    tk.Label(log_frame, text="Serial:", bg=HUD_BG, fg="#888", font=self._font).pack(
      anchor=tk.W
    )
    self._log_font = tkfont.Font(family="Consolas", size=9)
    if self._log_font.actual("family") == "TkFixedFont":
      self._log_font = tkfont.nametofont("TkFixedFont")
      self._log_font.configure(size=9)
    self.serial_log = tk.Text(
      log_frame,
      height=6,
      bg="#1a1a1a",
      fg="#ccc",
      font=self._log_font,
      wrap=tk.WORD,
      state=tk.DISABLED,
      relief=tk.FLAT,
      padx=6,
      pady=4,
    )
    self.serial_log.pack(fill=tk.X)
    self._log_line_count = 0

    self.canvas = tk.Frame(self.root, bg=OFF_SCREEN)
    self.canvas.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True)

  def _on_delay_changed(self, _value: str) -> None:
    ms = self.delay_var.get() * DELAY_STEP_MS
    sign = "+" if ms > 0 else ""
    self.delay_label.configure(text=f"{sign}{ms} мс")

  def _screen_delay_ms(self) -> int:
    return int(self.delay_var.get()) * DELAY_STEP_MS

  def _set_fill(self, color: str) -> None:
    self.canvas.configure(bg=color)

  def _update_serial_indicator(self) -> None:
    if self.bridge.connected:
      self.serial_led.configure(fg="#3d3")
      self.serial_label.configure(text=f"{self.bridge.port} @ {SERIAL_BAUD}")
    else:
      self.serial_led.configure(fg="#666")
      self.serial_label.configure(text="нет связи")

  def _refresh_ports(self, select: str | None = None) -> None:
    ports = list_serial_ports()
    values = [NO_PORT_LABEL, *ports]
    self.port_combo["values"] = values
    chosen = NO_PORT_LABEL
    if select and select in ports:
      chosen = select
    elif self.bridge.port and self.bridge.port in ports:
      chosen = self.bridge.port
    elif ports:
      chosen = ports[0]
    self.port_var.set(chosen)
    self._connect_port(chosen if chosen != NO_PORT_LABEL else None)

  def _refresh_ports_click(self) -> None:
    self._refresh_ports(self.port_var.get() if self.port_var.get() != NO_PORT_LABEL else None)

  def _on_port_selected(self, _event: tk.Event | None = None) -> None:
    val = self.port_var.get()
    self._connect_port(None if val == NO_PORT_LABEL else val)

  def _connect_port(self, port: str | None) -> None:
    self._stop_serial_poll()
    try:
      self.bridge.open(port)
    except (OSError, RuntimeError) as exc:
      self.bridge.close()
      self._update_serial_indicator()
      self.status.configure(text=f"Порт: {exc}")
      return
    self._update_serial_indicator()
    if self.bridge.connected:
      self._append_serial_log(f"# открыт {self.bridge.port} @ {SERIAL_BAUD}")
      self._start_serial_poll()

  def _append_serial_log(self, line: str) -> None:
    self.serial_log.configure(state=tk.NORMAL)
    self.serial_log.insert(tk.END, line + "\n")
    self._log_line_count += 1
    if self._log_line_count > SERIAL_LOG_LINES:
      self.serial_log.delete("1.0", "2.0")
      self._log_line_count -= 1
    self.serial_log.see(tk.END)
    self.serial_log.configure(state=tk.DISABLED)

  def _should_log_device_line(self, line: str) -> bool:
    return any(marker in line for marker in DEVICE_LOG_MARKERS)

  def _start_serial_poll(self) -> None:
    self._stop_serial_poll()
    if self.bridge.connected:
      self._serial_poll()

  def _stop_serial_poll(self) -> None:
    if self._serial_poll_after is not None:
      self.root.after_cancel(self._serial_poll_after)
      self._serial_poll_after = None

  def _serial_poll(self) -> None:
    if not self.bridge.connected:
      self._serial_poll_after = None
      return
    for line in self.bridge.poll_lines():
      if self._should_log_device_line(line):
        self._append_serial_log(line)
      self._process_device_line(line)
    self._serial_poll_after = self.root.after(40, self._serial_poll)

  def _process_device_line(self, line: str) -> None:
    m_abort = RE_ABORT.search(line)
    if m_abort:
      self._on_device_abort(m_abort.group(1).strip(), line)
      return
    m_fail = RE_CAL_FAIL.search(line)
    if m_fail:
      self._on_cal_fail(m_fail.group(1).strip(), line)
      return
    if RE_CAL_OK.search(line):
      if self._mode == "calibrating":
        self.status.configure(text="Устройство: калибровка OK — ждём конец фазы на экране")
      return
    if RE_RESULT.search(line) and self._mode == "measuring":
      self.status.configure(text="Устройство: замер завершён — см. [RESULT] в логе")

  def _on_device_abort(self, reason: str, line: str) -> None:
    """После прогрева: dRB low / ADC clip — замер на устройстве не стартует."""
    self._device_abort = True
    if self._mode in ("warming", "measuring"):
      self._cancel_all()
    self._mode = "idle"
    self._set_fill(OFF_SCREEN)
    self.status.configure(
      text=f"Устройство: замер отменён ({reason}). Проверьте сигнал / прижим датчика",
      fg="#f96",
    )

  def _on_cal_fail(self, reason: str, line: str) -> None:
    self._device_abort = True
    if self._mode == "calibrating":
      self._cancel_all()
      self._mode = "idle"
      self._set_fill(OFF_SCREEN)
    self.status.configure(text=f"Устройство: калибровка FAIL ({reason})", fg="#f96")

  # --- Таймеры и отложенная смена цвета --------------------------------------

  def _cancel_all(self) -> None:
    if self._tick_after is not None:
      self.root.after_cancel(self._tick_after)
      self._tick_after = None
    for aid in self._pending:
      try:
        self.root.after_cancel(aid)
      except tk.TclError:
        pass
    self._pending.clear()

  def _schedule_at(
    self,
    offset_ms: float,
    callback: Callable[[], None],
    *,
    from_timeline: bool = True,
    apply_delay: bool = False,
  ) -> None:
    """offset_ms от _timeline_start; apply_delay — сдвиг только для мигания в замере."""
    delay_extra = self._screen_delay_ms() if apply_delay else 0
    if from_timeline:
      target = self._timeline_start + (offset_ms + delay_extra) / 1000.0
    else:
      target = time.perf_counter() + (offset_ms + delay_extra) / 1000.0
    wait_ms = max(0.0, (target - time.perf_counter()) * 1000.0)
    holder: list[str | None] = [None]

    def wrapper() -> None:
      aid = holder[0]
      if aid is not None and aid in self._pending:
        self._pending.remove(aid)
      callback()

    holder[0] = self.root.after(int(wait_ms), wrapper)
    self._pending.append(holder[0])

  def _schedule_color(self, offset_ms: float, color: str, *, apply_delay: bool = False) -> None:
    self._schedule_at(offset_ms, lambda c=color: self._set_fill(c), apply_delay=apply_delay)

  # --- Serial + жесты ---------------------------------------------------------

  def _fire_serial(self, ch: str) -> None:
    if self.bridge.send_command(ch):
      self.status.configure(
        text=f">> [{ch}] отправлено на {self.bridge.port} + локальный стимул"
      )

  def enter_menu(self, msg: str = "Меню") -> None:
    self._cancel_all()
    self._mode = "idle"
    self._device_abort = False
    self._set_fill(OFF_SCREEN)
    self.status.configure(text=msg, fg=HUD_ACCENT)

  def _gesture_menu(self) -> None:
    self._fire_serial("m")
    self.enter_menu("Меню (как Single / serial m)")

  def _gesture_help(self) -> None:
    self.enter_menu(
      "m — меню | c — калибровка | r — прогрев+замер | "
      f"задержка экрана: шаг {DELAY_STEP_MS} мс"
    )

  def _gesture_calibration(self) -> None:
    self._device_abort = False
    self._fire_serial("c")
    self._start_calibration()

  def _gesture_measure(self) -> None:
    self._device_abort = False
    self._fire_serial("r")
    self._start_warmup_then_measure()

  # --- Последовательности (как Application.cpp) -------------------------------

  def _start_calibration(self) -> None:
    self._cancel_all()
    self._mode = "calibrating"
    self._timeline_start = time.perf_counter()
    total_ms = CALIB_PHASE_DURATION_MS * SCAN_PHASE_COUNT

    self._schedule_color(0, RED)
    self._schedule_color(CALIB_PHASE_DURATION_MS, BLUE)
    self._schedule_at(total_ms, lambda: self._finish_calibration(), from_timeline=True)

    self._arm_status_tick(
      "Калибровка",
      [(0, CALIB_PHASE_DURATION_MS, "Красный"), (CALIB_PHASE_DURATION_MS, total_ms, "Синий")],
      total_ms,
    )

  def _finish_calibration(self) -> None:
    if self._mode != "calibrating":
      return
    self._cancel_all()
    if self._device_abort:
      return
    self.enter_menu("Калибровка завершена (LED Off). c — снова, r — замер")

  def _start_warmup_then_measure(self) -> None:
    self._cancel_all()
    self._mode = "warming"
    self._timeline_start = time.perf_counter()
    warmup_total = WARMUP_PHASE_DURATION_MS * SCAN_PHASE_COUNT

    self._schedule_color(0, RED)
    self._schedule_color(WARMUP_PHASE_DURATION_MS, BLUE)
    self._schedule_at(warmup_total, self._warmup_done, from_timeline=True)

    self._arm_status_tick(
      "Прогрев",
      [
        (0, WARMUP_PHASE_DURATION_MS, "Красный"),
        (WARMUP_PHASE_DURATION_MS, warmup_total, "Синий"),
      ],
      warmup_total,
    )

  def _warmup_done(self) -> None:
    if self._mode != "warming":
      return
    if self._device_abort:
      self._cancel_all()
      self._mode = "idle"
      self._set_fill(OFF_SCREEN)
      return
    self._start_measure_phase()

  def _start_measure_phase(self) -> None:
    if self._mode not in ("warming", "measuring"):
      return
    if self._device_abort:
      return
    self._cancel_all()
    self._mode = "measuring"
    self._timeline_start = time.perf_counter()

    # Старт Red; переключения каждые BLINK_HALF_PERIOD_MS (как latLastToggleMs_).
    # Задержка экрана только здесь — прогрев/границы фаз совпадают с Arduino.
    self._schedule_color(0, RED, apply_delay=True)
    t = BLINK_HALF_PERIOD_MS
    while t < SESSION_MS:
      color = BLUE if (t // BLINK_HALF_PERIOD_MS) % 2 == 1 else RED
      self._schedule_color(t, color, apply_delay=True)
      t += BLINK_HALF_PERIOD_MS

    self._schedule_at(SESSION_MS, self._finish_measure, from_timeline=True)

    phases: list[tuple[int, int, str]] = []
    t0 = 0
    while t0 < SESSION_MS:
      t1 = min(t0 + BLINK_HALF_PERIOD_MS, SESSION_MS)
      name = "Красный" if (t0 // BLINK_HALF_PERIOD_MS) % 2 == 0 else "Синий"
      phases.append((t0, t1, name))
      t0 = t1

    self._arm_status_tick("Замер", phases, SESSION_MS)

  def _finish_measure(self) -> None:
    if self._mode != "measuring":
      return
    if self._device_abort:
      return
    self._cancel_all()
    self.enter_menu(
      f"Замер завершён ({SESSION_MS // 1000} с, полупериод {BLINK_HALF_PERIOD_MS} мс). "
      "Результаты на OLED/Serial устройства. m — меню, r — снова"
    )

  def _arm_status_tick(
    self,
    title: str,
    segments: list[tuple[int, int, str]],
    total_ms: int,
  ) -> None:
    def tick() -> None:
      if self._mode == "idle":
        return
      elapsed_ms = (time.perf_counter() - self._timeline_start) * 1000.0
      if elapsed_ms >= total_ms:
        return
      label = title
      for t0, t1, name in segments:
        if t0 <= elapsed_ms < t1:
          sec = max(0, int((t1 - elapsed_ms) / 1000.0) + 1)
          label = f"{title} — {name}, ~{sec} с"
          break
      if self._mode == "measuring":
        extra = self._screen_delay_ms()
        if extra:
          label += f" | задержка экрана +{extra} мс" if extra > 0 else f" | задержка экрана {extra} мс"
      self.status.configure(text=label)
      self._tick_after = self.root.after(100, tick)

    tick()

  # --- Привязки клавиш --------------------------------------------------------

  def bind_keys(self) -> None:
    for key in ("m", "M", "c", "C", "r", "R", "h", "H"):
      ch = key.lower()
      if ch == "m":
        self.root.bind(f"<{key}>", lambda e: self._gesture_menu())
      elif ch == "c":
        self.root.bind(f"<{key}>", lambda e: self._gesture_calibration())
      elif ch == "r":
        self.root.bind(f"<{key}>", lambda e: self._gesture_measure())
      elif ch == "h":
        self.root.bind(f"<{key}>", lambda e: self._gesture_help())
    self.root.bind("<?>", lambda e: self._gesture_help())
    self.root.bind("<Escape>", lambda e: self._quit())
    self.root.bind(
      "<F11>",
      lambda e: self.root.attributes(
        "-fullscreen", not self.root.attributes("-fullscreen")
      ),
    )

  def _quit(self) -> None:
    self._stop_serial_poll()
    self._cancel_all()
    self.bridge.close()
    self.root.destroy()


def parse_args() -> argparse.Namespace:
  p = argparse.ArgumentParser(description="Phototransistor screen stimulus (Arduino-synced)")
  p.add_argument(
    "--port",
    help="COM/tty (Windows: COM3, Linux: /dev/ttyACM0). Иначе выбор в списке.",
  )
  p.add_argument("--windowed", action="store_true", help="Не разворачивать на весь экран")
  return p.parse_args()


def main() -> int:
  if serial is None:
    print(
      "Предупреждение: pyserial не установлен — только локальный экран.\n"
      "  pip install -r requirements-screen-test.txt",
      file=sys.stderr,
    )

  args = parse_args()
  root = tk.Tk()
  app = StimulusApp(root, args.port)
  app.bind_keys()
  root.focus_set()
  if not args.windowed:
    root.attributes("-fullscreen", True)
  root.protocol("WM_DELETE_WINDOW", app._quit)
  root.mainloop()
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
