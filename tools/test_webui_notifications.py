#!/usr/bin/env python3
"""Source contracts for prominent, accessible WebUI notifications."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]


class WebUiNotificationContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.html = (ROOT / "webpanel/index.html").read_text(encoding="utf-8")
        cls.css = (ROOT / "webpanel/css/styles.css").read_text(encoding="utf-8")
        cls.js = (ROOT / "webpanel/js/index.js").read_text(encoding="utf-8")

    def test_notification_has_live_region_and_close_control(self) -> None:
        self.assertRegex(
            self.html,
            r'<div id="toast" role="status" aria-live="polite" aria-atomic="true">',
        )
        self.assertIn('id="toast-msg"', self.html)
        self.assertRegex(
            self.html,
            r'<button id="toast-close"[^>]+aria-label="Fechar notificação"',
        )

    def test_notification_is_prominent_near_top_and_mobile_safe(self) -> None:
        toast_css = re.search(r"#toast\s*\{(?P<body>.*?)\}", self.css, re.S)
        self.assertIsNotNone(toast_css)
        body = toast_css.group("body")
        self.assertIn("position: fixed", body)
        self.assertIn("top:", body)
        self.assertNotIn("bottom:", body)
        self.assertIn("max-width:", body)
        self.assertIn("env(safe-area-inset-top)", body)

    def test_errors_are_assertive_and_remain_longer_than_success(self) -> None:
        toast_fn = re.search(
            r"function toast\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}", self.js, re.S
        )
        self.assertIsNotNone(toast_fn)
        body = toast_fn.group("body")
        self.assertIn('isError ? "alert" : "status"', body)
        self.assertIn('isError ? "assertive" : "polite"', body)
        durations = re.search(r"isError\s*\?\s*(\d+)\s*:\s*(\d+)", body)
        self.assertIsNotNone(durations)
        self.assertGreater(int(durations.group(1)), int(durations.group(2)))
        self.assertIn("dismissToast", body)

    def test_close_button_is_wired_without_removing_inline_errors(self) -> None:
        self.assertIn('$("toast-close").onclick = dismissToast;', self.js)
        self.assertIn('toast(configError(e), "err");', self.js)
        self.assertIn('class="note" id="nf-msg"', self.html)
        self.assertIn('class="note" id="irr-msg"', self.html)


if __name__ == "__main__":
    unittest.main()
