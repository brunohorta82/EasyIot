#!/usr/bin/env python3
"""Source contracts for truthful runtime actuator persistence failures."""

from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]


def source(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def block_after(text: str, pattern: str, limit: int = 2400) -> str:
    match = re.search(pattern, text)
    if match is None:
        raise AssertionError(f"pattern not found: {pattern}")
    return text[match.start() : match.start() + limit]


class RuntimePersistenceContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = source("include/ConfigOnofre.h")
        cls.config = source("src/ConfigOnofre.cpp")
        cls.server = source("src/WebServer.cpp")
        cls.mqtt = source("src/Mqtt.cpp")
        cls.cloud = source("src/CloudIO.cpp")
        cls.actuators = source("src/Actuatores.cpp")
        cls.webui = source("webpanel/js/index.js")

    def test_control_api_returns_stable_results_and_legacy_wrapper_is_gone(self) -> None:
        self.assertNotRegex(self.header, r"ConfigOnofre\s*&\s*save\s*\(")
        self.assertNotRegex(self.config, r"ConfigOnofre::save\s*\(")
        for signature in (
            r"ConfigUpdateResult controlFeature\(StateOrigin origin, JsonObject &action",
            r"ConfigUpdateResult controlFeature\(StateOrigin origin, String topic",
            r"ConfigUpdateResult controlFeature\(StateOrigin origin, String uniqueId",
        ):
            self.assertRegex(self.header, signature)

    def test_runtime_change_reports_persist_failure_separately_from_unknown_id(self) -> None:
        control = block_after(
            self.config,
            r"ConfigUpdateResult\s+ConfigOnofre::controlFeature\(StateOrigin origin, String uniqueId",
        )
        self.assertLess(control.index("a.changeState(origin, state);"), control.index("persist()"))
        self.assertIn("ConfigUpdateResult::PERSISTENCE_FAILED", control)
        self.assertIn("ConfigUpdateResult::INVALID_REQUEST", control)

    def test_web_returns_applied_but_unsaved_as_507(self) -> None:
        start = self.server.index("/*CONTROL ACTUATOR*/")
        end = self.server.index("/*RESET A METER'S ACCUMULATED ENERGY*/", start)
        handler = self.server[start:end]
        self.assertIn("const ConfigUpdateResult result", handler)
        self.assertIn("ConfigUpdateResult::PERSISTENCE_FAILED", handler)
        self.assertIn('root["applied"] = true;', handler)
        self.assertIn("response->setCode(507);", handler)
        self.assertIn("reportControlPersistenceFailure", handler)
        self.assertIn("response->setCode(404);", handler)
        self.assertIn("Comando executado, mas o estado não ficou guardado", self.webui)

    def test_mqtt_and_cloud_do_not_discard_the_result(self) -> None:
        for text, origin in ((self.mqtt, "MQTT"), (self.cloud, "CLOUDIO")):
            self.assertRegex(
                text,
                rf"const\s+ConfigUpdateResult\s+result\s*=\s*config\.controlFeature\(StateOrigin::{origin}",
            )
            self.assertRegex(
                text,
                rf"result\s*==\s*ConfigUpdateResult::PERSISTENCE_FAILED[\s\S]{{0,180}}reportControlPersistenceFailure\(StateOrigin::{origin}",
            )

    def test_gpio_and_knx_persistence_failures_are_reported(self) -> None:
        gpio = block_after(self.actuators, r"void\s+controlFromGpio\s*\(")
        self.assertIn("const ConfigUpdateResult result", gpio)
        self.assertIn("ConfigUpdateResult::PERSISTENCE_FAILED", gpio)
        self.assertIn("reportControlPersistenceFailure(StateOrigin::GPIO_INPUT", gpio)
        self.assertGreaterEqual(self.actuators.count("controlFromGpio("), 9)

        knx = block_after(self.actuators, r"void\s+actuatoresCallback\s*\(")
        self.assertIn("if (!config.persist())", knx)
        self.assertIn("reportControlPersistenceFailure(StateOrigin::KNX", knx)

    def test_initial_and_discovery_saves_are_explicitly_checked(self) -> None:
        init = block_after(self.config, r"ConfigOnofre\s*&ConfigOnofre::init\s*\(")
        self.assertIn("if (!persist())", init)
        self.assertIn("configuracao inicial nao guardada", init)
        self.assertIn("descoberta PZEM nao guardada", self.config)
        self.assertIn("descoberta I2C nao guardada", self.config)


if __name__ == "__main__":
    unittest.main()
