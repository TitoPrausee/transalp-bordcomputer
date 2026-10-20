"""
mock_controller.py — Mock-Controller: Tastatur-Input -> Controller-Events

Simuliert den Lenker-Controller (HID Controller) ueber Tastatur-Input.
Tastatureingaben werden in controller_button_t-Events uebersetzt,
die der firmwareseitigen Definition entsprechen:
  firmware/components/hid_controller/include/controller_input.h
  firmware/components/hid_controller/include/controller_mapping.h

Button-IDs (controller_button_t):
  0=NAV_UP, 1=NAV_DOWN, 2=NAV_LEFT, 3=NAV_RIGHT,
  4=SELECT, 5=BACK, 6=HOME,
  7=VOL_UP, 8=VOL_DOWN, 9=MEDIA_NEXT, 10=MEDIA_PREV

Tastatur-Mapping:
  Pfeiltasten   -> NAV_UP/DOWN/LEFT/RIGHT
  Enter         -> SELECT
  Escape        -> BACK
  Pos1/Home     -> HOME
  +/-           -> VOL_UP/VOL_DOWN
  n/p           -> MEDIA_NEXT/MEDIA_PREV
"""

from __future__ import annotations

import time
from dataclasses import dataclass
from typing import List, Callable, Dict
from enum import IntEnum


# ---- Button-IDs (aus controller_input.h) ----

class ControllerButton(IntEnum):
    NONE = -1
    NAV_UP = 0
    NAV_DOWN = 1
    NAV_LEFT = 2
    NAV_RIGHT = 3
    SELECT = 4
    BACK = 5
    HOME = 6
    VOL_UP = 7
    VOL_DOWN = 8
    MEDIA_NEXT = 9
    MEDIA_PREV = 10


# ---- Event-IDs (aus controller_mapping.h) ----

class ControllerEvent(IntEnum):
    NAV_UP = 0
    NAV_DOWN = 1
    NAV_LEFT = 2
    NAV_RIGHT = 3
    BTN_SELECT = 4
    BTN_BACK = 5
    BTN_HOME = 6
    VOL_UP = 7
    VOL_DOWN = 8
    MEDIA_NEXT = 9
    MEDIA_PREV = 10


# Button -> Event Mapping (1:1 wie in firmware controller_mapping.c)
BUTTON_TO_EVENT: Dict[ControllerButton, ControllerEvent] = {
    ControllerButton.NAV_UP: ControllerEvent.NAV_UP,
    ControllerButton.NAV_DOWN: ControllerEvent.NAV_DOWN,
    ControllerButton.NAV_LEFT: ControllerEvent.NAV_LEFT,
    ControllerButton.NAV_RIGHT: ControllerEvent.NAV_RIGHT,
    ControllerButton.SELECT: ControllerEvent.BTN_SELECT,
    ControllerButton.BACK: ControllerEvent.BTN_BACK,
    ControllerButton.HOME: ControllerEvent.BTN_HOME,
    ControllerButton.VOL_UP: ControllerEvent.VOL_UP,
    ControllerButton.VOL_DOWN: ControllerEvent.VOL_DOWN,
    ControllerButton.MEDIA_NEXT: ControllerEvent.MEDIA_NEXT,
    ControllerButton.MEDIA_PREV: ControllerEvent.MEDIA_PREV,
}

BUTTON_NAMES: Dict[ControllerButton, str] = {
    ControllerButton.NAV_UP: "NAV_UP",
    ControllerButton.NAV_DOWN: "NAV_DOWN",
    ControllerButton.NAV_LEFT: "NAV_LEFT",
    ControllerButton.NAV_RIGHT: "NAV_RIGHT",
    ControllerButton.SELECT: "SELECT",
    ControllerButton.BACK: "BACK",
    ControllerButton.HOME: "HOME",
    ControllerButton.VOL_UP: "VOL_UP",
    ControllerButton.VOL_DOWN: "VOL_DOWN",
    ControllerButton.MEDIA_NEXT: "MEDIA_NEXT",
    ControllerButton.MEDIA_PREV: "MEDIA_PREV",
}

# Tastatur-Mapping (Key -> Button)
KEY_MAP: Dict[str, ControllerButton] = {
    "up": ControllerButton.NAV_UP,
    "down": ControllerButton.NAV_DOWN,
    "left": ControllerButton.NAV_LEFT,
    "right": ControllerButton.NAV_RIGHT,
    "return": ControllerButton.SELECT,
    "enter": ControllerButton.SELECT,
    "escape": ControllerButton.BACK,
    "home": ControllerButton.HOME,
    "+": ControllerButton.VOL_UP,
    "=": ControllerButton.VOL_UP,
    "-": ControllerButton.VOL_DOWN,
    "_": ControllerButton.VOL_DOWN,
    "n": ControllerButton.MEDIA_NEXT,
    "p": ControllerButton.MEDIA_PREV,
}

# Debounce (aus controller_input.h)
CTRL_DEBOUNCE_MS = 50


@dataclass
class ControllerEvent_:
    """Ein Controller-Event (Button-Press mit Zeitstempel)."""
    button: ControllerButton
    event: ControllerEvent
    pressed: bool
    timestamp_ms: int
    name: str = ""


class ButtonDebounce:
    """Debounce-State fuer einen einzelnen Button (aus controller_input.h)."""
    __slots__ = ("last_press_us", "last_state")

    def __init__(self):
        self.last_press_us: int = 0
        self.last_state: bool = False


class MockController:
    """
    Mock-Controller: Tastatur -> Controller-Events.

    Verwaltet Debounce-Logik und Event-Queue.
    Kann programmatisch Buttons injizieren (fuer Tests)
    oder ueber Tastatur-Input betrieben werden.

    Usage:
        ctrl = MockController()
        ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        events = ctrl.poll_events()
    """

    def __init__(self, debounce_ms: int = CTRL_DEBOUNCE_MS):
        self._debounce_ms = debounce_ms
        self._debounce: Dict[ControllerButton, ButtonDebounce] = {
            btn: ButtonDebounce() for btn in ControllerButton if btn >= 0
        }
        self._event_queue: List[ControllerEvent_] = []
        self._callbacks: List[Callable[[ControllerEvent_], None]] = []
        self._key_map = dict(KEY_MAP)

    def inject_button(self, button: ControllerButton, pressed: bool) -> bool:
        """
        Button-Event injizieren (mit Debounce-Filter).

        Entspricht firmware controller_input_process_button().
        Rueckgabe: True wenn akzeptiert (Debounce passiert), False sonst.
        """
        if button < 0 or button >= ControllerButton.MEDIA_PREV + 1:
            return False

        now_us = int(time.time() * 1_000_000)
        db = self._debounce[button]

        # Debounce pruefen (nur bei Press-Events)
        if pressed:
            if db.last_state:
                return False  # Bereits gedrueckt
            elapsed = now_us - db.last_press_us
            if elapsed < self._debounce_ms * 1000:
                return False  # Debounce-Fenster
            db.last_press_us = now_us
        else:
            if not db.last_state:
                return False  # Bereits losgelassen

        db.last_state = pressed

        # Nur bei Press ein Event erzeugen
        if pressed:
            event_id = BUTTON_TO_EVENT.get(button)
            if event_id is None:
                return False

            evt = ControllerEvent_(
                button=button,
                event=event_id,
                pressed=True,
                timestamp_ms=now_us // 1000,
                name=BUTTON_NAMES.get(button, "UNKNOWN"),
            )
            self._event_queue.append(evt)
            for cb in self._callbacks:
                cb(evt)

        return True

    def inject_key(self, key: str, pressed: bool = True) -> bool:
        """
        Tastatureingabe verarbeiten.

        key: Name der Taste (z.B. "up", "return", "escape")
        pressed: True = gedrueckt, False = losgelassen
        """
        key_lower = key.lower()
        button = self._key_map.get(key_lower)
        if button is None:
            return False
        return self.inject_button(button, pressed)

    def poll_events(self) -> List[ControllerEvent_]:
        """Alle ausstehenden Events abholen (und Queue leeren)."""
        events = self._event_queue
        self._event_queue = []
        return events

    @property
    def pending_events(self) -> int:
        return len(self._event_queue)

    def register_callback(self, cb: Callable[[ControllerEvent_], None]) -> None:
        """Callback fuer Button-Events registrieren."""
        self._callbacks.append(cb)

    def unregister_callback(self, cb: Callable[[ControllerEvent_], None]) -> None:
        if cb in self._callbacks:
            self._callbacks.remove(cb)

    def reset_button(self, button: ControllerButton) -> None:
        """Debounce-State fuer einen Button zuruecksetzen (fuer Tests)."""
        db = self._debounce.get(button)
        if db:
            db.last_press_us = 0
            db.last_state = False

    def reset_all(self) -> None:
        """Alle Debounce-States zuruecksetzen."""
        for db in self._debounce.values():
            db.last_press_us = 0
            db.last_state = False
        self._event_queue.clear()

    def would_accept(self, button: ControllerButton, now_us: int) -> bool:
        """Pruefen ob ein Button-Press akzeptiert wuerde (ohne zu registrieren)."""
        if button < 0:
            return False
        db = self._debounce[button]
        if db.last_state:
            return False
        elapsed = now_us - db.last_press_us
        return elapsed >= self._debounce_ms * 1000

    @property
    def key_map(self) -> Dict[str, ControllerButton]:
        return dict(self._key_map)

    def list_buttons(self) -> str:
        """Human-readable Button-Liste mit Tastatur-Mapping."""
        lines = ["Controller Button Mapping:"]
        reverse_map: Dict[ControllerButton, List[str]] = {}
        for key, btn in self._key_map.items():
            reverse_map.setdefault(btn, []).append(key)
        for btn in ControllerButton:
            if btn < 0:
                continue
            keys = reverse_map.get(btn, [])
            key_str = ", ".join(keys) if keys else "(none)"
            lines.append(f"  {BUTTON_NAMES.get(btn, 'UNKNOWN'):15s} [{btn:2d}]  Key: {key_str}")
        return "\n".join(lines)
