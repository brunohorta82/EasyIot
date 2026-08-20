#!/usr/bin/env python3
"""Source-contract tests for captive-portal credential submission."""

from __future__ import annotations

from pathlib import Path
import ast
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
CAPTIVE_HEADER = ROOT / "include" / "CaptivePortal.h"
WEB_SERVER = ROOT / "src" / "WebServer.cpp"


class CaptivePortalTests(unittest.TestCase):
    def test_base_page_uses_one_small_buffer_and_compact_brand(self) -> None:
        header = CAPTIVE_HEADER.read_text(encoding="utf-8")
        server = WEB_SERVER.read_text(encoding="utf-8")

        self.assertIn(
            'beginResponseStream("text/html", 4096)',
            server,
        )
        self.assertIn("FPSTR(HTTP_CAPTIVE_BODY_START)", server)
        self.assertNotIn("FPSTR(HTTP_HEADER_END)", server)

        match = re.search(
            r"const char HTTP_CAPTIVE_BODY_START\[\] PROGMEM =\s*"
            r"((?:\s*\"(?:[^\"\\]|\\.)*\")+)\s*;",
            header,
        )
        self.assertIsNotNone(match)
        fragments = re.findall(r'\"(?:[^\"\\]|\\.)*\"', match.group(1))
        compact_brand = "".join(ast.literal_eval(fragment) for fragment in fragments)
        self.assertLess(len(compact_brand), 1024)
        self.assertIn("ONOFRE", compact_brand)

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
        self.assertIn("bool isRequestHandlerTrivial()", source)
        self.assertIn("bool isRequestHandlerTrivial() const", source)
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
        self.assertIn("response->print(FPSTR(HTTP_CAPTIVE_INVALID));", source)

    def test_wifi_repair_preserves_an_existing_template(self) -> None:
        source = WEB_SERVER.read_text(encoding="utf-8")

        self.assertIn(
            "if (config.templateId == Template::NO_TEMPLATE)",
            source,
        )
        self.assertIn("invalidTemplate = templateParam == nullptr;", source)
        self.assertIn("templateValue != String(templateId)", source)
        self.assertIn("!isCaptiveTemplateAllowed(templateId)", source)
        self.assertIn("!config.loadTemplate(templateId)", source)

    def test_captive_template_whitelist_excludes_han_only_choice(self) -> None:
        source = WEB_SERVER.read_text(encoding="utf-8")
        helper = re.search(
            r"bool isCaptiveTemplateAllowed\(int templateId\)\s*\{(.*?)\n\}",
            source,
            re.DOTALL,
        )
        self.assertIsNotNone(helper)
        body = helper.group(1)
        for template in (
            "NO_TEMPLATE",
            "DUAL_LIGHT",
            "DUAL_SWITCH",
            "COVER",
            "GARAGE",
            "GARDEN",
        ):
            self.assertIn(f"case Template::{template}:", body)
        self.assertNotIn("case Template::HAN_MODULE:", body)


if __name__ == "__main__":
    unittest.main(verbosity=2)
