#!/usr/bin/env python3
"""Source-contract tests for captive-portal credential submission."""

from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
CAPTIVE_HEADER = ROOT / "include" / "CaptivePortal.h"
WEB_SERVER = ROOT / "src" / "WebServer.cpp"


class CaptivePortalTests(unittest.TestCase):
    def test_all_captive_forms_submit_credentials_with_post(self) -> None:
        source = CAPTIVE_HEADER.read_text(encoding="utf-8")
        methods = re.findall(r"<form method='([^']+)' action='/'", source)

        self.assertEqual(methods, ["post", "post"])
        self.assertNotIn("method='get'", source)

    def test_configuration_mutation_requires_post(self) -> None:
        source = WEB_SERVER.read_text(encoding="utf-8")

        self.assertIn(
            "const bool isSubmission = request->method() == HTTP_POST;", source
        )
        self.assertIn("bool isRequestHandlerTrivial() override", source)
        self.assertIn("return false;", source)
        self.assertIn(
            'request->getParam("s", true)',
            source,
        )
        self.assertIn('request->getParam("i", true)', source)
        self.assertIn('request->getParam("p", true)', source)
        self.assertIn('request->getParam("t", true)', source)
        self.assertNotIn('request->arg("s")', source)
        self.assertNotIn('request->arg("p")', source)
        self.assertIn(
            'if (!isSubmission && request->hasArg("sc"))',
            source,
        )

    def test_captive_response_is_not_cached_and_invalid_post_fails(self) -> None:
        source = WEB_SERVER.read_text(encoding="utf-8")

        self.assertIn('response->addHeader("Cache-Control", "no-store");', source)
        self.assertIn("response->setCode(400);", source)
        self.assertIn("response->print(FPSTR(HTTP_INVALID));", source)


if __name__ == "__main__":
    unittest.main(verbosity=2)
