#!/usr/bin/env python3
"""Host-only tests for the ESP8266 static-RAM budget guard."""

from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).with_name("check_ram_budget.py")
SPEC = importlib.util.spec_from_file_location("check_ram_budget", SCRIPT)
assert SPEC and SPEC.loader
check_ram_budget = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check_ram_budget)


class RamBudgetTests(unittest.TestCase):
    def test_parses_platformio_esp8266_ram_sections(self) -> None:
        output = """firmware.elf  :
section  size  addr
.data  1616  1073643520
.rodata  16180  1073645200
.bss  34392  1073661384
Total  900000
"""
        self.assertEqual(
            check_ram_budget.parse_section_sizes(output, (".data", ".rodata", ".bss")),
            {".data": 1616, ".rodata": 16180, ".bss": 34392},
        )

    def test_rejects_missing_required_section(self) -> None:
        with self.assertRaisesRegex(check_ram_budget.BudgetError, r"missing required sections: \.bss"):
            check_ram_budget.parse_section_sizes(
                ".data 100 1\n.rodata 200 2\n", (".data", ".rodata", ".bss")
            )

    def test_report_passes_at_limit_and_shows_baseline_delta(self) -> None:
        passed, report = check_ram_budget.budget_report(
            "ESP8266_RELEASE",
            {".data": 1000, ".rodata": 2000, ".bss": 7000},
            {"baseline_bytes": 9500, "limit_bytes": 10000},
            "9.200",
        )
        self.assertTrue(passed)
        self.assertIn("10,000 / 10,000 bytes", report)
        self.assertIn("+500 vs v9.200 baseline", report)

    def test_report_fails_over_limit_with_review_instruction(self) -> None:
        passed, report = check_ram_budget.budget_report(
            "ESP8266_RELEASE",
            {".data": 1000, ".rodata": 2000, ".bss": 7001},
            {"baseline_bytes": 9500, "limit_bytes": 10000},
            "9.200",
        )
        self.assertFalse(passed)
        self.assertIn("[FAIL]", report)
        self.assertIn("-1 margin", report)
        self.assertIn("Do not raise the limit", report)

    def test_budget_file_requires_exact_protected_environment_set(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "budgets.json"
            path.write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "baseline_version": "9.200",
                        "ram_sections": [".data", ".rodata", ".bss"],
                        "environments": {
                            "ESP8266_RELEASE": {"baseline_bytes": 1, "limit_bytes": 2}
                        },
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(check_ram_budget.BudgetError, "missing"):
                check_ram_budget.load_budgets(path)

    def test_repository_budget_baselines_fit_limits(self) -> None:
        baseline_version, sections, environments = check_ram_budget.load_budgets()
        self.assertEqual(baseline_version, "9.200")
        self.assertEqual(sections, (".data", ".rodata", ".bss"))
        self.assertEqual(set(environments), check_ram_budget.EXPECTED_ENVIRONMENTS)
        for budget in environments.values():
            self.assertLessEqual(budget["baseline_bytes"], budget["limit_bytes"])

    def test_candidate_publication_checks_budget_first(self) -> None:
        source = Path(__file__).with_name("post_extra_script.py").read_text(encoding="utf-8")
        function_start = source.index("def export_firmware_candidate")
        check_call = source.index("check_esp8266_ram_budget(env)", function_start)
        publish_message = source.index("Publishing local firmware candidate", function_start)
        self.assertLess(check_call, publish_message)

    def test_release_workflow_has_explicit_esp8266_budget_gate(self) -> None:
        workflow = (Path(__file__).parents[1] / ".github" / "workflows" / "firmware-ota.yml").read_text(
            encoding="utf-8"
        )
        self.assertIn("name: Verify ESP8266 static-RAM budget", workflow)
        self.assertIn("python3 tools/check_ram_budget.py --env ${{ matrix.env }}", workflow)


if __name__ == "__main__":
    unittest.main(verbosity=2)
