"""conftest.py — pytest Konfiguration fuer Simulator-Tests"""

import sys
import os

# Simulator-Verzeichnis zum Python-Pfad hinzufuegen
SIM_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SIM_DIR not in sys.path:
    sys.path.insert(0, SIM_DIR)

# Track-Verzeichnis
TRACKS_DIR = os.path.join(SIM_DIR, "tracks")
SAMPLE_TRACK = os.path.join(TRACKS_DIR, "sample_alps.gpx")
