"""
conftest.py — Pytest configuration for integration tests.

Adds the simulator root directory to sys.path so that mock_*.py
modules can be imported from the integration test directory.
"""

import os
import sys

# Add simulator root to path
SIM_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if SIM_ROOT not in sys.path:
    sys.path.insert(0, SIM_ROOT)
