#!/usr/bin/env python3
"""Source contracts for the WebUI Pinout row indicators."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class WebUiPinoutContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.css = (ROOT / "webpanel/css/styles.css").read_text(encoding="utf-8")
        cls.js = (ROOT / "webpanel/js/index.js").read_text(encoding="utf-8")

    def test_pin_rows_share_the_same_edge_structure(self) -> None:
        self.assertIn("border-left-width: 3px", self.css)

    def test_edges_match_existing_pin_categories(self) -> None:
        self.assertIn(".pin-out { border-left-color: var(--grn); }", self.css)
        self.assertIn(".pin-in { border-left-color: var(--blu); }", self.css)
        self.assertIn(".pin-free { border-left-color: var(--dim); }", self.css)

    def test_renderer_keeps_category_text_and_adds_matching_row_class(self) -> None:
        self.assertIn('const pinState = u ? u.k : "free";', self.js)
        self.assertIn('<span class="tag t-out">SAÍDA</span>', self.js)
        self.assertIn('<span class="tag t-in">ENTRADA</span>', self.js)
        self.assertIn('<span class="tag t-free">LIVRE</span>', self.js)
        self.assertIn('class="pin pin-\' + pinState + (right ? " r" : "")', self.js)


if __name__ == "__main__":
    unittest.main()
