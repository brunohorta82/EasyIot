#!/usr/bin/env python3
"""The LED strip's cloud id must be unique across devices, like every other feature's."""

from __future__ import annotations

from pathlib import Path
import re
import unittest

SOURCE = (Path(__file__).resolve().parent.parent / "src" / "LedStrip.cpp").read_text()


def body(signature: str) -> str:
    start = SOURCE.index(signature)
    return SOURCE[start:SOURCE.index("\n}\n", start)]


class LedCloudIdTests(unittest.TestCase):
    def test_cloud_id_starts_with_the_chip_id(self) -> None:
        # The cloud looks a feature up by id across all devices: a bare "led"
        # would let a second strip anywhere take over the first one's feature.
        helper = body("static const char *cloudStripId()")
        self.assertIn('"%sled", config.chipId', helper)

    def test_cloud_payload_and_topics_use_that_id(self) -> None:
        self.assertIn('a["id"] = cloudStripId();', body("void ledStripFeatureJson("))
        topics = body("void ledStripCloudTopics(")
        self.assertIn('"%s/%s/light/%s/set", username, chipId, cloudStripId()', topics)
        self.assertIn('"%s/%s/light/%s/status", username, chipId, cloudStripId()', topics)
        self.assertNotRegex(topics, re.escape("/light/led/"))

    def test_the_panel_keeps_its_local_id(self) -> None:
        # The panel, local MQTT and Home Assistant are per device: unchanged.
        self.assertIn('o["id"] = "led";', body("void ledStripConfigJson("))


if __name__ == "__main__":
    unittest.main()
