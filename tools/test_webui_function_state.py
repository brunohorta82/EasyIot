#!/usr/bin/env python3
"""Source contracts for live binary state in configured Function cards."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]


class WebUiFunctionStateContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.css = (ROOT / "webpanel/css/styles.css").read_text(encoding="utf-8")
        cls.js = (ROOT / "webpanel/js/index.js").read_text(encoding="utf-8")

    def test_only_truthful_gpio_binary_drivers_receive_live_state(self) -> None:
        self.assertIn(
            'const BINARY_STATE_DRIVERS = ["LIGHT_PUSH", "LIGHT_LATCH", "SWITCH_PUSH", "SWITCH_LATCH"];',
            self.js,
        )
        helper = re.search(
            r"const hasBinaryFunctionState = \(f\) =>(?P<body>.*?);",
            self.js,
            re.S,
        )
        self.assertIsNotNone(helper)
        self.assertIn("isActuator(f)", helper.group("body"))
        self.assertIn("!isVirtual(f)", helper.group("body"))

    def test_function_cards_keep_text_and_matching_edge(self) -> None:
        self.assertIn(".feature-card.on { border-left-color: var(--grn); }", self.css)
        self.assertIn(".feature-state.off", self.css)
        self.assertIn(".feature-state.on", self.css)
        self.assertIn('badge.textContent = on ? "LIGADO" : "DESLIGADO";', self.js)

    def test_sse_updates_the_configured_card(self) -> None:
        self.assertIn("const current = applyFeatureState(feat.id, e.data) || feat;", self.js)
        self.assertIn('card.classList.toggle("on", on);', self.js)
        self.assertIn('$("fc-" + feature.id)', self.js)
        self.assertIn('$("fs-" + feature.id)', self.js)

    def test_web_control_updates_shared_state_after_device_accepts_it(self) -> None:
        self.assertIn("async function control(id, state, confirmedState)", self.js)
        self.assertIn("if (confirmedState != null) applyFeatureState(id, confirmedState);", self.js)
        self.assertIn("control(sw.dataset.toggle, 102, on ? 100 : 0);", self.js)

    def test_opening_functions_reconciles_state_without_rebuilding_edits(self) -> None:
        refresh = re.search(
            r"async function refreshFeatureStates\(\)(?P<body>.*?\n})",
            self.js,
            re.S,
        )
        self.assertIsNotNone(refresh)
        self.assertIn('api("/config?featureState=" + Date.now(), { cache: "no-store" })', refresh.group("body"))
        self.assertIn("applyFeatureState(fresh.id, fresh.state)", refresh.group("body"))
        self.assertNotIn("renderFeatures()", refresh.group("body"))
        self.assertIn('if (tab.dataset.view === "features") refreshFeatureStates();', self.js)


if __name__ == "__main__":
    unittest.main()
