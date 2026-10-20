"""
test_mock_controller.py — Tests fuer den Mock-Controller

Testet:
  1. Button-Injection und Event-Generierung
  2. Debounce-Logik
  3. Tastatur-Mapping
  4. Event-Queue
  5. Callback-Registrierung
  6. Reset-Funktionen
"""

import time
from mock_controller import (
    MockController, ControllerButton, ControllerEvent,
    CTRL_DEBOUNCE_MS,
)


class TestButtonInjection:
    """Test der Button-Injection."""

    def test_inject_nav_up(self):
        """NAV_UP Button generiert ein Event."""
        ctrl = MockController()
        accepted = ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        assert accepted is True
        events = ctrl.poll_events()
        assert len(events) == 1
        assert events[0].button == ControllerButton.NAV_UP
        assert events[0].event == ControllerEvent.NAV_UP
        assert events[0].pressed is True

    def test_inject_all_buttons(self):
        """Alle Buttons generieren Events."""
        ctrl = MockController()
        for btn in ControllerButton:
            if btn < 0:
                continue
            ctrl.reset_all()
            accepted = ctrl.inject_button(btn, pressed=True)
            assert accepted is True, f"Button {btn} not accepted"
            events = ctrl.poll_events()
            assert len(events) == 1
            assert events[0].button == btn

    def test_release_does_not_generate_event(self):
        """Button-Release generiert kein Event."""
        ctrl = MockController()
        ctrl.inject_button(ControllerButton.SELECT, pressed=True)
        ctrl.poll_events()
        ctrl.inject_button(ControllerButton.SELECT, pressed=False)
        events = ctrl.poll_events()
        assert len(events) == 0


class TestDebounce:
    """Test der Debounce-Logik."""

    def test_double_press_within_debounce_rejected(self):
        """Zweiter Press innerhalb Debounce-Zeit wird abgelehnt."""
        ctrl = MockController(debounce_ms=CTRL_DEBOUNCE_MS)
        assert ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        # Sofort nochmal -> abgelehnt
        assert not ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        events = ctrl.poll_events()
        assert len(events) == 1  # Nur das erste Event

    def test_press_after_debounce_accepted(self):
        """Press nach Ablauf der Debounce-Zeit wird akzeptiert."""
        ctrl = MockController(debounce_ms=10)
        ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        ctrl.poll_events()
        ctrl.inject_button(ControllerButton.NAV_UP, pressed=False)
        time.sleep(0.015)  # 15ms > 10ms debounce
        accepted = ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        assert accepted is True

    def test_different_buttons_not_debounced(self):
        """Verschiedene Buttons werden unabhaengig debounced."""
        ctrl = MockController()
        assert ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        assert ctrl.inject_button(ControllerButton.NAV_DOWN, pressed=True)
        events = ctrl.poll_events()
        assert len(events) == 2

    def test_would_accept(self):
        """would_accept prueft ohne zu registrieren."""
        ctrl = MockController()
        ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        now_us = int(time.time() * 1_000_000)
        # Innerhalb Debounce -> nicht akzeptiert
        assert not ctrl.would_accept(ControllerButton.NAV_UP, now_us)


class TestKeyMapping:
    """Test des Tastatur-Mappings."""

    def test_arrow_up_maps_to_nav_up(self):
        """Pfeil-oben -> NAV_UP."""
        ctrl = MockController()
        ctrl.inject_key("up")
        events = ctrl.poll_events()
        assert len(events) == 1
        assert events[0].button == ControllerButton.NAV_UP

    def test_enter_maps_to_select(self):
        """Enter -> SELECT."""
        ctrl = MockController()
        ctrl.inject_key("return")
        events = ctrl.poll_events()
        assert len(events) == 1
        assert events[0].button == ControllerButton.SELECT

    def test_escape_maps_to_back(self):
        """Escape -> BACK."""
        ctrl = MockController()
        ctrl.inject_key("escape")
        events = ctrl.poll_events()
        assert len(events) == 1
        assert events[0].button == ControllerButton.BACK

    def test_unknown_key_ignored(self):
        """Unbekannte Taste wird ignoriert."""
        ctrl = MockController()
        accepted = ctrl.inject_key("f12")
        assert accepted is False
        assert ctrl.pending_events == 0

    def test_case_insensitive(self):
        """Tasten-Eingabe ist case-insensitive."""
        ctrl = MockController()
        ctrl.inject_key("N")
        events = ctrl.poll_events()
        assert len(events) == 1
        assert events[0].button == ControllerButton.MEDIA_NEXT


class TestEventQueue:
    """Test der Event-Queue."""

    def test_poll_clears_queue(self):
        """poll_events leert die Queue."""
        ctrl = MockController()
        ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        ctrl.inject_button(ControllerButton.NAV_DOWN, pressed=True)
        events = ctrl.poll_events()
        assert len(events) == 2
        assert ctrl.poll_events() == []

    def test_pending_events_count(self):
        """pending_events gibt die Anzahl der ausstehenden Events."""
        ctrl = MockController()
        assert ctrl.pending_events == 0
        ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        assert ctrl.pending_events == 1


class TestCallbacks:
    """Test der Callback-Funktionen."""

    def test_register_callback(self):
        """Callback wird bei Button-Event aufgerufen."""
        ctrl = MockController()
        received = []
        ctrl.register_callback(lambda evt: received.append(evt))
        ctrl.inject_button(ControllerButton.SELECT, pressed=True)
        assert len(received) == 1
        assert received[0].button == ControllerButton.SELECT

    def test_unregister_callback(self):
        """Abgemeldeter Callback wird nicht aufgerufen."""
        ctrl = MockController()
        received = []
        cb = lambda evt: received.append(evt)
        ctrl.register_callback(cb)
        ctrl.unregister_callback(cb)
        ctrl.inject_button(ControllerButton.SELECT, pressed=True)
        assert len(received) == 0


class TestReset:
    """Test der Reset-Funktionen."""

    def test_reset_button(self):
        """reset_button setzt den Debounce-State zurueck."""
        ctrl = MockController()
        ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        ctrl.inject_button(ControllerButton.NAV_UP, pressed=False)
        ctrl.reset_button(ControllerButton.NAV_UP)
        accepted = ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        assert accepted is True

    def test_reset_all(self):
        """reset_all setzt alle States zurueck."""
        ctrl = MockController()
        ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        ctrl.inject_button(ControllerButton.NAV_DOWN, pressed=True)
        ctrl.reset_all()
        assert ctrl.pending_events == 0
        # Beide Buttons wieder akzeptierbar
        assert ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        assert ctrl.inject_button(ControllerButton.NAV_DOWN, pressed=True)


class TestButtonListing:
    """Test der Button-Listing-Ausgabe."""

    def test_list_buttons(self):
        """list_buttons gibt lesbaren String zurueck."""
        ctrl = MockController()
        listing = ctrl.list_buttons()
        assert "NAV_UP" in listing
        assert "SELECT" in listing
        assert "BACK" in listing
        assert "MEDIA_NEXT" in listing
