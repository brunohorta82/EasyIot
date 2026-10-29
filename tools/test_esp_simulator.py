#!/usr/bin/env python3
"""Tests for the ESP simulator and mock server.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest
from urllib.request import urlopen, Request
import threading
import time

ROOT = Path(__file__).resolve().parents[1]
SIMULATOR_FILE = ROOT / "tools" / "esp_simulator.py"


class TestEspSimulator(unittest.TestCase):
    def test_simulator_file_exists(self) -> None:
        self.assertTrue(SIMULATOR_FILE.exists(), "tools/esp_simulator.py must exist")

    def test_simulator_imports_and_has_routes(self) -> None:
        content = SIMULATOR_FILE.read_text(encoding="utf-8")
        self.assertIn("class MockEspHandler", content)
        self.assertIn('"/config"', content)
        self.assertIn('"/aquadance"', content)
        self.assertIn('"/aquadance-run"', content)
        self.assertIn('"/aquadance-stop"', content)
        self.assertIn('"/control"', content)


if __name__ == "__main__":
    unittest.main()
