#!/usr/bin/env python3
"""Source-contract tests for the AquaDance / Fontaine musical matrix feature,
including lighting/dimming/RGBW, 2D fountain simulation, and Home Assistant mapping.
"""

from __future__ import annotations

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
AQUADANCE_HEADER = ROOT / "include" / "AquaDance.h"
AQUADANCE_SOURCE = ROOT / "src" / "AquaDance.cpp"
CONFIG_SOURCE = ROOT / "src" / "ConfigOnofre.cpp"
CONSTANTS_HEADER = ROOT / "include" / "Constants.h"
HA_DISCOVERY_HEADER = ROOT / "include" / "HomeAssistantMqttDiscovery.h"
HA_DISCOVERY_SOURCE = ROOT / "src" / "HomeAssistantMqttDiscovery.cpp"
TEMPLATES_SOURCE = ROOT / "src" / "Templates.cpp"
WEB_SERVER = ROOT / "src" / "WebServer.cpp"
WEB_PANEL = ROOT / "webpanel" / "js" / "index.js"
WEB_PANEL_HTML = ROOT / "webpanel" / "index.html"
WEB_PANEL_CSS = ROOT / "webpanel" / "css" / "styles.css"


class TestAquaDanceSourceContracts(unittest.TestCase):
    def test_aquadance_header_exists_and_declares_engine(self) -> None:
        self.assertTrue(AQUADANCE_HEADER.exists(), "AquaDance.h header must exist")
        content = AQUADANCE_HEADER.read_text(encoding="utf-8")
        self.assertIn("class AquaDance", content)
        self.assertIn("struct AquaTrack", content)
        self.assertIn("struct AquaShow", content)
        self.assertIn("enum AquaTrackType", content)
        self.assertIn("TRACK_VALVE", content)
        self.assertIn("TRACK_LIGHT_DIMMER", content)
        self.assertIn("TRACK_LIGHT_RGBW", content)
        self.assertIn("bool play(uint8_t showId);", content)
        self.assertIn("void stop();", content)
        self.assertIn("void loop();", content)
        self.assertIn("extern AquaDance aquadance;", content)

    def test_aquadance_source_implements_safe_controls_and_fixtures(self) -> None:
        self.assertTrue(AQUADANCE_SOURCE.exists(), "AquaDance.cpp must exist")
        content = AQUADANCE_SOURCE.read_text(encoding="utf-8")
        self.assertIn("bool AquaDance::play(uint8_t showId)", content)
        self.assertIn("void AquaDance::stop()", content)
        self.assertIn("void AquaDance::loop()", content)
        self.assertIn("findFixture", content)
        self.assertIn("persistJsonAtomically(configFilenames::aquadance", content)

    def test_constants_header_defines_aquadance_file(self) -> None:
        content = CONSTANTS_HEADER.read_text(encoding="utf-8")
        self.assertIn('constexpr const char *aquadance = "/aquadance.json";', content)

    def test_config_onofre_loads_and_loops_aquadance(self) -> None:
        content = CONFIG_SOURCE.read_text(encoding="utf-8")
        self.assertIn('#include "AquaDance.h"', content)
        self.assertIn("aquadance.load();", content)
        self.assertIn("aquadance.loop();", content)

    def test_home_assistant_discovery_supports_aquadance(self) -> None:
        header = HA_DISCOVERY_HEADER.read_text(encoding="utf-8")
        source = HA_DISCOVERY_SOURCE.read_text(encoding="utf-8")
        self.assertIn("void createHaAquaDance();", header)
        self.assertIn("void publishAquaDanceHomeAssistantState();", header)
        self.assertIn("void createHaAquaDance()", source)
        self.assertIn("publishAquaDanceHomeAssistantState()", source)
        self.assertIn("_aquadance_running", source)
        self.assertIn("_aquadance_stop", source)

    def test_templates_stops_aquadance_on_graph_clear(self) -> None:
        content = TEMPLATES_SOURCE.read_text(encoding="utf-8")
        self.assertIn('#include "AquaDance.h"', content)
        self.assertIn("if (aquadance.isRunning())", content)
        self.assertIn("aquadance.stop();", content)

    def test_webserver_registers_aquadance_endpoints(self) -> None:
        content = WEB_SERVER.read_text(encoding="utf-8")
        self.assertIn('#include "AquaDance.h"', content)
        self.assertIn('"/aquadance"', content)
        self.assertIn('"/aquadance/run"', content)
        self.assertIn('"/aquadance/stop"', content)
        self.assertIn('"/aquadance-run"', content)
        self.assertIn('"/aquadance-stop"', content)

    def test_webpanel_html_contains_aquadance_matrix_and_2d_pool(self) -> None:
        content = WEB_PANEL_HTML.read_text(encoding="utf-8")
        self.assertIn('id="btn-sub-dance"', content)
        self.assertIn('id="pane-irr-dance"', content)
        self.assertIn('id="dance-matrix-wrap"', content)
        self.assertIn('id="dance-pool-basin"', content)
        self.assertIn('id="dance-export-ha"', content)
        self.assertIn('id="dance-swatches"', content)
        self.assertIn('id="dance-play"', content)
        self.assertIn('id="dance-stop"', content)

    def test_webpanel_js_contains_matrix_drawing_and_2d_simulation(self) -> None:
        content = WEB_PANEL_JS = WEB_PANEL.read_text(encoding="utf-8")
        self.assertIn("function renderAquaDanceTab()", content)
        self.assertIn("function renderDanceMatrix()", content)
        self.assertIn("function renderDancePoolBasin()", content)
        self.assertIn("function exportHomeAssistant2DCard()", content)
        self.assertIn("function applyPoolLayoutPreset(", content)
        self.assertIn("function applyDancePreset(", content)
        self.assertIn("runAquaDanceShow()", content)
        self.assertIn("stopAquaDanceShow()", content)
        self.assertIn("active-jet", content)
        self.assertIn("active-light", content)

    def test_webpanel_css_contains_aquadance_and_pool_styles(self) -> None:
        content = WEB_PANEL_CSS.read_text(encoding="utf-8")
        self.assertIn(".dance-matrix", content)
        self.assertIn(".dance-cell", content)
        self.assertIn(".dance-cell.on", content)
        self.assertIn(".dance-pool-basin", content)
        self.assertIn(".pool-node", content)
        self.assertIn(".pool-node.active-jet", content)
        self.assertIn(".pool-node.is-light", content)


if __name__ == "__main__":
    unittest.main()
