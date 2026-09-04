#!/usr/bin/env python3
"""Source-contract tests for the Presence & Radar 2D Visualizer and Studio,
including mmWave radars (HLK-LD2410, LD2450, LD2460), 2D target tracking,
detection zones, virtual simulation, and Home Assistant export.
"""

from __future__ import annotations

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SENSORS_HEADER = ROOT / "include" / "Sensors.h"
SENSORS_SOURCE = ROOT / "src" / "Sensors.cpp"
CONSTANTS_HEADER = ROOT / "include" / "Constants.h"
HA_DISCOVERY_SOURCE = ROOT / "src" / "HomeAssistantMqttDiscovery.cpp"
WEB_PANEL = ROOT / "webpanel" / "js" / "index.js"
WEB_PANEL_HTML = ROOT / "webpanel" / "index.html"
WEB_PANEL_CSS = ROOT / "webpanel" / "css" / "styles.css"


class TestPresenceRadarSourceContracts(unittest.TestCase):
    def test_sensors_header_declares_radar_drivers(self) -> None:
        self.assertTrue(SENSORS_HEADER.exists(), "Sensors.h header must exist")
        content = SENSORS_HEADER.read_text(encoding="utf-8")
        self.assertIn("LD2410 = 94", content)
        self.assertIn("LD2450 = 96", content)
        self.assertIn("LD2460 = 97", content)

    def test_sensors_source_implements_radar_parsers(self) -> None:
        self.assertTrue(SENSORS_SOURCE.exists(), "Sensors.cpp must exist")
        content = SENSORS_SOURCE.read_text(encoding="utf-8")
        self.assertIn("case LD2410:", content)
        self.assertIn("case LD2450:", content)
        self.assertIn("case LD2460:", content)
        self.assertIn("stationaryTargetDistance", content)
        self.assertIn("movingTargetDistance", content)
        self.assertIn("LD2450Parser", content)
        self.assertIn("LD2460Parser", content)
        self.assertIn('prefix + "x"', content)
        self.assertIn('prefix + "y"', content)

    def test_constants_defines_presence_payloads(self) -> None:
        content = CONSTANTS_HEADER.read_text(encoding="utf-8")
        self.assertIn('constexpr const char *presenceOnPayload{"detected"};', content)
        self.assertIn('constexpr const char *presenceOffPayload{"clear"};', content)
        self.assertIn('constexpr const char *LD2410{"LD2410"};', content)
        self.assertIn('constexpr const char *LD2450{"LD2450"};', content)
        self.assertIn('constexpr const char *LD2460{"LD2460"};', content)

    def test_webpanel_html_contains_radar_studio_elements(self) -> None:
        content = WEB_PANEL_HTML.read_text(encoding="utf-8")
        self.assertIn('id="ov-radar-title"', content)
        self.assertIn('id="ov-radar-studio"', content)
        self.assertIn('id="radar-sensor-select"', content)
        self.assertIn('id="radar-canvas-wrap"', content)
        self.assertIn('id="radar-svg"', content)
        self.assertIn('id="radar-zones-group"', content)
        self.assertIn('id="radar-targets-group"', content)
        self.assertIn('id="radar-trails-group"', content)
        self.assertIn('id="radar-ld2410-card"', content)
        self.assertIn('id="radar-btn-sim"', content)
        self.assertIn('id="radar-btn-export-ha"', content)

    def test_webpanel_js_implements_radar_studio_logic(self) -> None:
        content = WEB_PANEL.read_text(encoding="utf-8")
        self.assertIn("function renderRadarStudio()", content)
        self.assertIn("function renderRadarStudioState(", content)
        self.assertIn("function renderRadarTargetsSvg(", content)
        self.assertIn("function renderRadarTargetsTable(", content)
        self.assertIn("function renderRadarZones()", content)
        self.assertIn("function checkZoneOccupancy(", content)
        self.assertIn("function toggleRadarSimulation()", content)
        self.assertIn("function exportRadarHomeAssistantCard()", content)
        self.assertIn("handleRadarLiveEvent(", content)
        self.assertIn("RADAR_DRIVERS", content)

    def test_webpanel_css_contains_radar_studio_styles(self) -> None:
        content = WEB_PANEL_CSS.read_text(encoding="utf-8")
        self.assertIn(".radar-studio-card", content)
        self.assertIn(".radar-canvas-wrap", content)
        self.assertIn(".radar-svg", content)
        self.assertIn(".radar-grid-arc", content)
        self.assertIn(".radar-zone-shape", content)
        self.assertIn(".radar-target-dot", content)
        self.assertIn(".radar-trail-dot", content)
        self.assertIn(".radar-gauge-bar", content)


if __name__ == "__main__":
    unittest.main()
