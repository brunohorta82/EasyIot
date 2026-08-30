#!/usr/bin/env python3
"""Source contracts for truthful, accessible WebUI health indicators."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]


class WebUiHealthContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.html = (ROOT / "webpanel/index.html").read_text(encoding="utf-8")
        cls.css = (ROOT / "webpanel/css/styles.css").read_text(encoding="utf-8")
        cls.js = (ROOT / "webpanel/js/index.js").read_text(encoding="utf-8")

    def test_health_badges_keep_text_and_have_dedicated_indicator_class(self) -> None:
        self.assertIn('<div class="card status-card" id="d-links-card">', self.html)
        for element_id in ("d-net-status", "d-mqtt", "d-cloud", "d-clock"):
            self.assertRegex(
                self.html,
                rf'class="status-badge health-badge off" id="{element_id}">[^<]+<',
            )
        self.assertNotRegex(
            self.html,
            r'class="[^"]*health-badge[^"]*" id="s-name-dirty"',
        )

    def test_informational_cards_share_structure_without_claiming_health(self) -> None:
        self.assertRegex(
            self.html,
            r'<div class="card status-card off">\s*<h4>MEMÓRIA LIVRE</h4>',
        )
        self.assertRegex(
            self.html,
            r'<div class="card status-card off">\s*<h4>SISTEMA</h4>',
        )

    def test_health_edges_cover_all_four_meanings(self) -> None:
        self.assertIn(".pill.off { border-left: 3px solid var(--dim); }", self.css)
        self.assertIn(".status-card.off { border-left-color: var(--dim); }", self.css)
        expected = {
            "ok": "var(--status-ok)",
            "warn": "var(--status-warn)",
            "bad": "var(--status-bad)",
            "off": "var(--dim)",
        }
        for state, colour in expected.items():
            self.assertIn(
                f".health-badge.{state} {{ border-left-color: {colour}; }}",
                self.css,
            )

    def test_runtime_badge_updates_preserve_health_indicator(self) -> None:
        setter = re.search(
            r"function setStatusBadge\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            self.js,
            re.S,
        )
        self.assertIsNotNone(setter)
        self.assertIn('"status-badge health-badge " + state', setter.group("body"))
        self.assertIn("el.textContent = text", setter.group("body"))

    def test_weak_wifi_is_degraded_not_a_false_failure(self) -> None:
        classifier = re.search(
            r"function rssiClass\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            self.js,
            re.S,
        )
        self.assertIsNotNone(classifier)
        body = classifier.group("body")
        self.assertIn('return "off"', body)
        self.assertIn('signal >= -70 ? "ok" : "warn"', body)
        self.assertNotIn('"bad"', body)
        self.assertIn('config.signal < -80 ? "muito fraco" : "fraco"', self.js)

    def test_disabled_services_remain_neutral_and_failures_remain_red(self) -> None:
        self.assertIn('const state = !configured ? "off" : on ? "ok" : "bad";', self.js)
        self.assertIn('const cloudState = !cloud ? "off" : config.cloudConnected ? "ok" : "bad";', self.js)
        self.assertIn('config.clockSynced ? "ok" : "warn"', self.js)

    def test_connections_card_uses_worst_active_state(self) -> None:
        self.assertIn('const linkStates = [clockState, cloudState];', self.js)
        self.assertIn(
            'if ((config.mqttIpDns || "").trim()) linkStates.push(config.mqttConnected ? "ok" : "bad");',
            self.js,
        )
        self.assertRegex(
            self.js,
            r'const linksState = linkStates\.includes\("bad"\) \? "bad" :\s*'
            r'linkStates\.includes\("warn"\) \? "warn" : '
            r'linkStates\.includes\("ok"\) \? "ok" : "off";',
        )
        self.assertIn('$("d-links-card").className = "card status-card " + linksState;', self.js)


if __name__ == "__main__":
    unittest.main()
