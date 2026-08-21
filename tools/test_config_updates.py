#!/usr/bin/env python3
"""Source-contract tests for safe, truthful configuration pin updates.

These tests deliberately inspect the production sources.  EasyIot does not yet
have a native Arduino/AsyncWebServer harness, so this suite protects the design
boundaries that firmware builds alone cannot make visible.  It does not pretend
to execute GPIO, LittleFS, HTTP, or FreeRTOS behavior.
"""

from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
CONFIG_HEADER = ROOT / "include" / "ConfigOnofre.h"
CONFIG_SOURCE = ROOT / "src" / "ConfigOnofre.cpp"
CONSTANTS_HEADER = ROOT / "include" / "Constants.h"
ACTUATOR_HEADER = ROOT / "include" / "Actuatores.h"
ACTUATOR_SOURCE = ROOT / "src" / "Actuatores.cpp"
SENSOR_HEADER = ROOT / "include" / "Sensors.h"
SENSOR_SOURCE = ROOT / "src" / "Sensors.cpp"
MAIN_SOURCE = ROOT / "src" / "main.cpp"
TEMPLATES_SOURCE = ROOT / "src" / "Templates.cpp"
WEB_SERVER = ROOT / "src" / "WebServer.cpp"
WEB_PANEL = ROOT / "webpanel" / "js" / "index.js"
WEB_PANEL_HTML = ROOT / "webpanel" / "index.html"
PERSISTENCE_SOURCE = ROOT / "src" / "Persistence.cpp"
IRRIGATION_SOURCE = ROOT / "src" / "Irrigation.cpp"


def block_after(source: str, pattern: str) -> str:
    """Return the balanced brace block following *pattern*.

    The scanner ignores comments and quoted strings so JSON examples, log text,
    and JavaScript strings cannot accidentally terminate a function early.
    """

    match = re.search(pattern, source, flags=re.MULTILINE)
    if match is None:
        raise AssertionError(f"source contract not found: {pattern}")
    opening = source.find("{", match.end())
    if opening < 0:
        raise AssertionError(f"opening brace not found after: {pattern}")

    depth = 0
    state = "code"
    quote = ""
    escaped = False
    index = opening
    while index < len(source):
        char = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""

        if state == "line_comment":
            if char == "\n":
                state = "code"
        elif state == "block_comment":
            if char == "*" and following == "/":
                state = "code"
                index += 1
        elif state == "string":
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                state = "code"
        elif char == "/" and following == "/":
            state = "line_comment"
            index += 1
        elif char == "/" and following == "*":
            state = "block_comment"
            index += 1
        elif char in ('"', "'", "`"):
            state = "string"
            quote = char
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1 : index]
        index += 1

    raise AssertionError(f"closing brace not found after: {pattern}")


def esp32_branches(block: str) -> tuple[str, str]:
    """Return ESP32 and non-ESP32 branches from a simple conditional block."""

    match = re.search(
        r"#ifdef\s+ESP32\s*(.*?)#else\s*(.*?)#endif",
        block,
        flags=re.DOTALL,
    )
    if match is None:
        raise AssertionError("expected an ESP32/non-ESP32 conditional")
    return match.group(1), match.group(2)


class ConfigUpdateSourceContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = CONFIG_HEADER.read_text(encoding="utf-8")
        cls.config = CONFIG_SOURCE.read_text(encoding="utf-8")
        cls.constants = CONSTANTS_HEADER.read_text(encoding="utf-8")
        cls.actuator_header = ACTUATOR_HEADER.read_text(encoding="utf-8")
        cls.actuator = ACTUATOR_SOURCE.read_text(encoding="utf-8")
        cls.sensor_header = SENSOR_HEADER.read_text(encoding="utf-8")
        cls.sensor = SENSOR_SOURCE.read_text(encoding="utf-8")
        cls.main = MAIN_SOURCE.read_text(encoding="utf-8")
        cls.templates = TEMPLATES_SOURCE.read_text(encoding="utf-8")
        cls.server = WEB_SERVER.read_text(encoding="utf-8")
        cls.panel = WEB_PANEL.read_text(encoding="utf-8")
        cls.panel_html = WEB_PANEL_HTML.read_text(encoding="utf-8")
        cls.persistence = PERSISTENCE_SOURCE.read_text(encoding="utf-8")
        cls.irrigation = IRRIGATION_SOURCE.read_text(encoding="utf-8")

    def assertOrdered(self, source: str, *needles: str) -> None:  # noqa: N802
        cursor = -1
        for needle in needles:
            found = source.find(needle, cursor + 1)
            self.assertGreater(
                found,
                cursor,
                f"expected {needle!r} after offset {cursor}",
            )
            cursor = found

    def irrigationValidationPrefix(self) -> tuple[str, str]:  # noqa: N802
        """Return Irrigation::update and everything before its first mutation."""

        update = block_after(
            self.irrigation, r"bool\s+Irrigation::update\s*\("
        )
        mutation_patterns = (
            r"(?m)^\s*(?:this->)?enabled\s*=",
            r"(?m)^\s*(?:this->)?skipOnRain\s*=",
            r"(?m)^\s*if\s*\(\s*isRunning\(\)\s*\)",
            r"(?m)^\s*programs\s*=",
        )
        mutation_offsets = []
        for pattern in mutation_patterns:
            match = re.search(pattern, update)
            self.assertIsNotNone(match, f"missing irrigation commit step: {pattern}")
            assert match is not None
            mutation_offsets.append(match.start())
        return update, update[: min(mutation_offsets)]

    def test_result_codes_are_stable_and_update_can_report_failure(self) -> None:
        enum_body = block_after(
            self.header, r"enum\s+class\s+ConfigUpdateResult\s*:\s*uint8_t"
        )
        codes = {
            name: int(value)
            for name, value in re.findall(r"\b([A-Z_]+)\s*=\s*(\d+)", enum_body)
        }
        self.assertEqual(
            codes,
            {
                "OK": 0,
                "INVALID_REQUEST": 1,
                "INVALID_PIN": 2,
                "PIN_COUNT_MISMATCH": 3,
                "PIN_CONFLICT": 4,
                "BUSY": 5,
                "PERSISTENCE_FAILED": 6,
            },
        )
        self.assertIn(
            "ConfigUpdateResult update(JsonObject &root, JsonVariant &responseRoot);",
            self.header,
        )
        self.assertNotIn("ConfigOnofre &update(JsonObject &root);", self.header)

    def test_feature_access_lease_is_atomic_only_on_esp32(self) -> None:
        marker = self.header.index(
            "std::atomic<bool> featureAccessInProgress"
        )
        conditional_start = self.header.rfind("#ifdef ESP32", 0, marker)
        conditional_end = self.header.find("#endif", marker)
        self.assertGreaterEqual(conditional_start, 0)
        self.assertGreater(conditional_end, marker)
        conditional = self.header[
            conditional_start : conditional_end + len("#endif")
        ]
        esp32, esp8266 = esp32_branches(conditional)

        self.assertIn(
            "std::atomic<bool> featureAccessInProgress{false};", esp32
        )
        self.assertIn(
            "std::atomic<uint32_t> featureAccessYieldUntilMs{0};", esp32
        )
        self.assertNotIn("std::atomic", esp8266)
        self.assertRegex(
            esp8266,
            r"bool\s+featureAccessInProgress\s*(?:=\s*false|\{false\})\s*;",
        )
        self.assertRegex(
            esp8266,
            r"uint32_t\s+featureAccessYieldUntilMs\s*=\s*0\s*;",
        )

    def test_pin_capabilities_are_role_aware_and_portable(self) -> None:
        output_check = block_after(
            self.header, r"bool\s+validOutputPin\s*\(\s*unsigned\s+int\s+pin\s*\)\s+const"
        )
        input_check = block_after(
            self.header, r"bool\s+validInputPin\s*\(\s*unsigned\s+int\s+pin\s*\)\s+const"
        )
        compatibility_check = block_after(
            self.header, r"bool\s+validPin\s*\(\s*unsigned\s+int\s+pin\s*\)\s+const"
        )

        self.assertIn("DefaultPins::outputInputPins", output_check)
        self.assertNotIn("DefaultPins::intputOnlyPins", output_check)
        self.assertIn("validOutputPin(pin)", input_check)
        self.assertRegex(
            input_check,
            r"#if\s+defined\(ESP32\)\s*&&\s*!defined\(ESP32C6\)",
        )
        self.assertIn("DefaultPins::intputOnlyPins", input_check)
        self.assertIn("return validOutputPin(pin);", compatibility_check)

        classic_esp32 = self.constants[
            self.constants.index("#if defined(ESP32) && !defined(ESP32C6)") :
            self.constants.index("#ifdef ESP32C6")
        ]
        esp32c6 = self.constants[
            self.constants.index("#ifdef ESP32C6") :
            self.constants.index("#ifdef ESP8266")
        ]
        self.assertIn("intputOnlyPins[] = {34, 35, 36, 37, 38}", classic_esp32)
        self.assertNotIn("intputOnlyPins", esp32c6)
        c6_pins = re.search(
            r"outputInputPins\[\]\s*=\s*\{([^}]*)\}", esp32c6
        )
        self.assertIsNotNone(c6_pins)
        configurable_c6_pins = {
            int(pin.strip()) for pin in c6_pins.group(1).split(",") if pin.strip()
        }
        self.assertNotIn(6, configurable_c6_pins)
        self.assertNotIn(7, configurable_c6_pins)
        self.assertIn(4, configurable_c6_pins)
        self.assertIn(5, configurable_c6_pins)

    def test_stored_actuators_fail_closed_before_invalid_pins_are_driven(self) -> None:
        load = block_after(self.config, r"ConfigOnofre\s*&\s*ConfigOnofre::load\s*\(")
        actuator = load[load.index('strcmp(d["group"] | "", "ACTUATOR")') :]
        self.assertOrdered(
            actuator,
            "validOutputPin(output)",
            "validInputPin(input)",
            "if (!storedPinsValid)",
            "continue;",
            "actuator.setup();",
        )
        invalid = block_after(actuator, r"if\s*\(\s*!storedPinsValid\s*\)")
        self.assertNotIn("actuator.setup();", invalid)

    def test_new_template_features_validate_input_capable_pins(self) -> None:
        prepare = block_after(self.templates, r"int\s+prepareNewFeature\s*\(")
        virtual = block_after(self.templates, r"int\s+prepareVirtualSwitch\s*\(")
        self.assertGreaterEqual(prepare.count("config.validSensorPin("), 2)
        self.assertGreaterEqual(virtual.count("config.validInputPin("), 2)
        self.assertNotIn("config.validPin(", prepare)
        self.assertNotIn("config.validPin(", virtual)

    def test_sensor_input_count_is_a_driver_contract(self) -> None:
        expected_count = block_after(
            self.sensor_header,
            r"static\s+size_t\s+expectedInputCount\s*\(",
        )
        self.assertRegex(
            expected_count,
            r"case\s+DHT_11\s*:\s*"
            r"case\s+DHT_21\s*:\s*"
            r"case\s+DHT_22\s*:\s*"
            r"case\s+PIR\s*:\s*"
            r"case\s+RAIN\s*:\s*"
            r"case\s+DOOR\s*:\s*"
            r"case\s+WINDOW\s*:\s*"
            r"case\s+DS18B20\s*:\s*"
            r"return\s+1\s*;",
        )
        self.assertRegex(
            expected_count,
            r"case\s+PZEM_004T_V03\s*:\s*"
            r"case\s+PZEM_004T_V01\s*:\s*"
            r"case\s+HAN\s*:\s*"
            r"case\s+LTR303X\s*:\s*"
            r"case\s+SHT4X\s*:\s*"
            r"case\s+TMF882X\s*:\s*"
            r"case\s+HCSR04\s*:\s*"
            r"case\s+LD2410\s*:\s*"
            r"return\s+2\s*;",
        )
        self.assertRegex(
            expected_count,
            r"case\s+INVALID_SENSOR\s*:\s*default\s*:\s*return\s+0\s*;",
        )

    def test_ld2410_template_preserves_uart_rx_tx_slot_order(self) -> None:
        template = block_after(
            self.templates,
            r"void\s+prepareLD2410\s*\(\s*String\s+name\s*,\s*"
            r"unsigned\s+int\s+rx\s*,\s*unsigned\s+int\s+tx\s*\)",
        )
        creation = block_after(self.templates, r"int\s+prepareNewFeature\s*\(")
        sensor_loop = block_after(self.sensor, r"void\s+Sensor::loop\s*\(\s*\)")
        ld2410_loop = sensor_loop[
            sensor_loop.index("case LD2410:") : sensor_loop.index(
                "#endif", sensor_loop.index("case LD2410:")
            )
        ]

        self.assertIn("sensor.inputs = {rx, tx};", template)
        self.assertNotIn("sensor.inputs = {tx, rx};", template)
        self.assertIn("prepareLD2410(name, input1, input2);", creation)
        self.assertRegex(
            ld2410_loop,
            r"Serial1\.begin\(256000,\s*SERIAL_8N1,\s*inputs\[0\],\s*"
            r"inputs\[1\]\)",
        )

    def test_new_sensor_creation_rejects_unsupported_drivers(self) -> None:
        prepare = block_after(self.templates, r"int\s+prepareNewFeature\s*\(")
        validation_gate = prepare[: prepare.index("if (!config.validSensorPin")]
        for driver in (
            "DHT_11",
            "DHT_21",
            "DHT_22",
            "DS18B20",
            "DOOR",
            "WINDOW",
            "PIR",
            "HCSR04",
            "RAIN",
            "LD2410",
            "PZEM_004T_V03",
            "PZEM_004T_V01",
        ):
            self.assertIn(f"case SensorDriver::{driver}:", validation_gate)
        self.assertRegex(
            validation_gate,
            r"default\s*:\s*return\s+3\s*;\s*\}",
        )
        self.assertGreaterEqual(prepare.count("default:"), 2)
        self.assertGreaterEqual(prepare.count("return 3;"), 2)

    def test_hcsr04_case_cannot_fall_through_into_han(self) -> None:
        sensor_loop = block_after(self.sensor, r"void\s+Sensor::loop\s*\(\s*\)")
        hcsr04 = sensor_loop[
            sensor_loop.index("case HCSR04:") : sensor_loop.index("case HAN:")
        ]
        self.assertRegex(hcsr04, r"}\s*break\s*;\s*$")

    def test_sensor_runtime_topology_guards_fail_closed_before_pin_access(self) -> None:
        topology = block_after(
            self.sensor_header,
            r"bool\s+hasRuntimeInputTopology\s*\(\s*\)\s+const",
        )
        sensor_loop = block_after(self.sensor, r"void\s+Sensor::loop\s*\(\s*\)")

        self.assertRegex(
            topology,
            r"case\s+DHT_11\s*:[\s\S]*?case\s+HCSR04\s*:\s*"
            r"return\s+inputs\.size\(\)\s*==\s*expectedInputCount\(driver\)",
        )
        self.assertRegex(topology, r"default\s*:\s*return\s+false\s*;")
        self.assertOrdered(
            sensor_loop,
            "if (!ready)",
            "if (!hasRuntimeInputTopology())",
            "setError();",
            "return;",
            "if (!wifiConnected())",
            "switch (driver)",
        )

    def test_sensor_pin_validation_matches_the_electrical_role(self) -> None:
        validator = block_after(
            self.header,
            r"bool\s+validSensorPin\s*\(\s*SensorDriver\s+driver",
        )
        self.assertRegex(
            validator,
            r"case\s+SensorDriver::PIR\s*:\s*"
            r"return\s+slot\s*==\s*0\s*&&\s*validInputPin",
        )
        self.assertRegex(
            validator,
            r"case\s+SensorDriver::HCSR04\s*:\s*"
            r"return\s+slot\s*<\s*2\s*&&\s*\(slot\s*==\s*0\s*\?\s*"
            r"validOutputPin\(pin\)\s*:\s*validInputPin",
        )
        self.assertRegex(
            validator,
            r"case\s+SensorDriver::LD2410\s*:\s*"
            r"case\s+SensorDriver::PZEM_004T_V03\s*:\s*"
            r"case\s+SensorDriver::PZEM_004T_V01\s*:\s*"
            r"return\s+slot\s*<\s*2\s*&&\s*\(slot\s*==\s*0\s*\?\s*"
            r"validInputPin\(pin\)\s*:\s*validOutputPin",
        )
        han = validator[
            validator.index("case SensorDriver::HAN:") :
            validator.index("case SensorDriver::DHT_11:")
        ]
        esp32_han, esp8266_han = esp32_branches(han)
        self.assertRegex(
            esp32_han,
            r"slot\s*==\s*0\s*\?\s*validInputPin\(pin\)\s*:\s*"
            r"validOutputPin",
        )
        self.assertRegex(
            esp8266_han,
            r"slot\s*==\s*0\s*\?\s*validOutputPin\(pin\)\s*:\s*"
            r"validInputPin",
        )
        self.assertRegex(
            validator,
            r"case\s+SensorDriver::DHT_11\s*:\s*"
            r"case\s+SensorDriver::DHT_21\s*:\s*"
            r"case\s+SensorDriver::DHT_22\s*:\s*"
            r"case\s+SensorDriver::RAIN\s*:\s*"
            r"case\s+SensorDriver::DOOR\s*:\s*"
            r"case\s+SensorDriver::WINDOW\s*:\s*"
            r"case\s+SensorDriver::DS18B20\s*:\s*"
            r"return\s+slot\s*==\s*0\s*&&\s*validOutputPin",
        )
        self.assertRegex(
            validator,
            r"case\s+SensorDriver::LTR303X\s*:\s*"
            r"case\s+SensorDriver::SHT4X\s*:\s*"
            r"case\s+SensorDriver::TMF882X\s*:\s*"
            r"return\s+slot\s*<\s*2\s*&&\s*validOutputPin",
        )
        self.assertRegex(
            validator,
            r"case\s+SensorDriver::INVALID_SENSOR\s*:\s*default\s*:"
            r"\s*return\s+false",
        )

        claim = block_after(self.config, r"struct\s+PinClaim")
        role_check = block_after(self.config, r"bool\s+validPinForRole\s*\(")
        self.assertIn("SensorDriver sensorDriver", claim)
        self.assertIn("cfg.validSensorPin(claim.sensorDriver", role_check)

    def test_fixed_runtime_sensors_claim_the_physical_bus_and_reject_fake_remaps(self) -> None:
        fixed = block_after(
            self.sensor_header,
            r"static\s+bool\s+fixedRuntimeInputs\s*\(",
        )
        self.assertRegex(
            fixed,
            r"case\s+LTR303X\s*:\s*case\s+SHT4X\s*:\s*"
            r"case\s+TMF882X\s*:[\s\S]*?DefaultPins::SDA[\s\S]*?"
            r"DefaultPins::SCL[\s\S]*?return\s+true",
        )
        self.assertRegex(
            fixed,
            r"case\s+HAN\s*:[\s\S]*?SensorRuntimePins::HAN_RX[\s\S]*?"
            r"SensorRuntimePins::HAN_TX[\s\S]*?return\s+true",
        )
        self.assertRegex(
            fixed,
            r"case\s+PZEM_004T_V01\s*:[\s\S]*?"
            r"SensorRuntimePins::PZEM_V01_RX[\s\S]*?"
            r"SensorRuntimePins::PZEM_V01_TX[\s\S]*?return\s+true",
        )

        in_use = block_after(
            self.header, r"bool\s+pinInUse\s*\(",
        )
        self.assertOrdered(
            in_use,
            "Sensor::fixedRuntimeInputs(s.driver, claimedInputs)",
            "claimedInputs = s.inputs;",
            "for (auto p : claimedInputs)",
        )

        preflight = block_after(
            self.config, r"ConfigUpdateResult\s+preparePinUpdate\s*\(",
        )
        sensor_plan = block_after(
            preflight,
            r"for\s*\(\s*const\s+auto\s*&sensor\s*:\s*cfg\.sensors\s*\)",
        )
        self.assertOrdered(
            sensor_plan,
            "Sensor::fixedRuntimeInputs(sensor.driver, plan.oldInputs)",
            "plan.oldInputs = sensor.inputs;",
            "plan.inputs = plan.oldInputs;",
        )
        self.assertRegex(
            preflight,
            r"Sensor::fixedRuntimeInputs\(plan->sensorDriver,\s*fixedInputs\)\s*&&\s*"
            r"pins\s*!=\s*fixedInputs[\s\S]*?"
            r"return\s+ConfigUpdateResult::INVALID_PIN",
        )

    def test_sensor_target_capability_fails_closed_in_validation_and_runtime(self) -> None:
        capability = block_after(
            self.sensor_header,
            r"static\s+bool\s+isSupportedOnCurrentTarget\s*\(",
        )
        self.assertIn("case INVALID_SENSOR:", capability)
        self.assertRegex(capability, r"default\s*:\s*return\s+false")
        self.assertRegex(
            capability,
            r"#ifdef\s+ESP8266[\s\S]*?TMF882X[\s\S]*?LD2410[\s\S]*?"
            r"return\s+false",
        )
        self.assertRegex(
            capability,
            r"CONFIG_IDF_TARGET_ESP32C6[\s\S]*?CONFIG_IDF_TARGET_ESP32C3"
            r"[\s\S]*?PZEM_004T_V01[\s\S]*?return\s+false",
        )

        validator = block_after(
            self.header,
            r"bool\s+validSensorPin\s*\(",
        )
        topology = block_after(
            self.sensor_header,
            r"bool\s+hasRuntimeInputTopology\s*\(\s*\)\s+const",
        )
        preflight = block_after(
            self.config, r"ConfigUpdateResult\s+preparePinUpdate\s*\(",
        )
        self.assertIn("Sensor::isSupportedOnCurrentTarget", validator)
        self.assertIn("isSupportedOnCurrentTarget(driver)", topology)
        self.assertIn("Sensor::isSupportedOnCurrentTarget", preflight)

    def test_i2c_discovery_is_deferred_deduplicated_and_saved_once(self) -> None:
        sensor_loop = block_after(self.sensor, r"void\s+Sensor::loop\s*\(\s*\)")
        pzem = sensor_loop[
            sensor_loop.index("case PZEM_004T_V03:") :
            sensor_loop.index("case PZEM_004T_V01:")
        ]
        self.assertIn("config.requestI2cDiscovery();", pzem)
        self.assertNotIn("config.i2cDiscovery();", pzem)

        service = block_after(
            self.config,
            r"void\s+ConfigOnofre::serviceDeferredI2cDiscovery\s*\(\s*\)",
        )
        self.assertOrdered(
            service,
            "i2cDiscoveryRequested.exchange(false",
            "if (!tryBeginFeatureAccess())",
            "requestI2cDiscovery();",
            "i2cDiscovery();",
            "endFeatureAccess();",
        )
        features_task = block_after(self.main, r"void\s+featuresTask\s*\(")
        self.assertOrdered(
            features_task,
            "config.loopSensors();",
            "config.serviceDeferredI2cDiscovery();",
        )
        main_loop = block_after(self.main, r"void\s+loop\s*\(\s*\)")
        self.assertOrdered(
            main_loop,
            "config.loopSensors();",
            "config.serviceDeferredI2cDiscovery();",
        )

        discovery = block_after(
            self.config, r"void\s+ConfigOnofre::i2cDiscovery\s*\(\s*\)",
        )
        self.assertEqual(discovery.count("save();"), 1)
        self.assertIn("bool needsSave = false;", discovery)
        tmf_start = discovery.index("Discovery::I2C_TMF880X_ADDRESS")
        tmf = discovery[tmf_start:]
        self.assertOrdered(
            tmf,
            "if (!isSensorExists(address))",
            "prepareTMF882X(address);",
            "needsSave = true;",
        )
        self.assertIn("if (display == nullptr)", discovery)
        self.assertIn("delete candidate;", discovery)

    def test_sensor_pin_changes_are_inert_until_controlled_restart(self) -> None:
        preflight = block_after(
            self.config, r"ConfigUpdateResult\s+preparePinUpdate\s*\("
        )
        self.assertRegex(
            preflight,
            r"for\s*\(\s*auto\s*&\s*plan\s*:\s*plans\s*\)\s*"
            r"if\s*\(\s*plan\.kind\s*==\s*FeatureKind::SENSOR\s*\)\s*"
            r"plan\.restartRequired\s*=\s*plan\.oldInputs\s*!=\s*plan\.inputs",
        )

        update = block_after(
            self.config, r"ConfigUpdateResult\s+ConfigOnofre::update\s*\("
        )
        self.assertOrdered(
            update,
            "sensor->deactivateForConfigUpdate();",
            "releaseChangedOutputs",
            "sensor.inputs = plan->inputs;",
            "restartRequired = true;",
        )
        self.assertIn("bool ready = true;", self.sensor_header)
        sensor_loop = block_after(self.sensor, r"void\s+Sensor::loop\s*\(\s*\)")
        self.assertOrdered(sensor_loop, "if (!ready)", "if (!wifiConnected())")
        config_loop = block_after(
            self.config, r"void\s+ConfigOnofre::loopSensors\s*\(\s*\)"
        )
        self.assertOrdered(
            config_loop,
            "if (!tryBeginFeatureLoopAccess())",
            "if (!sensor.ready)",
            "endFeatureAccess();",
            "return;",
            "s.loop();",
            "endFeatureAccess();",
        )

    def test_feature_access_lease_is_non_reentrant_and_non_blocking(self) -> None:
        begin = block_after(
            self.config, r"bool\s+ConfigOnofre::tryBeginFeatureAccess\s*\(\s*\)"
        )
        esp32, esp8266 = esp32_branches(begin)
        self.assertOrdered(
            esp32,
            "bool expected = false;",
            "featureAccessInProgress.compare_exchange_strong",
            "return false;",
            "return true;",
        )
        self.assertIn("std::memory_order_acquire", esp32)
        self.assertIn("std::memory_order_relaxed", esp32)
        self.assertOrdered(
            esp8266,
            "if (featureAccessInProgress)",
            "return false;",
            "featureAccessInProgress = true;",
            "return true;",
        )
        for branch in (esp32, esp8266):
            self.assertNotRegex(branch, r"\bwhile\s*\(")
            self.assertNotRegex(branch, r"\b(?:delay|yield|vTaskDelay)\s*\(")

        end = block_after(
            self.config, r"void\s+ConfigOnofre::endFeatureAccess\s*\(\s*\)"
        )
        end_esp32, end_esp8266 = esp32_branches(end)
        self.assertIn(
            "featureAccessInProgress.store(false, std::memory_order_release)",
            end_esp32,
        )
        self.assertIn("featureAccessInProgress = false;", end_esp8266)

    def test_single_feature_access_lease_replaces_the_old_nested_gate(self) -> None:
        public_api = self.header[: self.header.index("private:")]
        self.assertIn("bool tryBeginFeatureAccess();", public_api)
        self.assertIn("void endFeatureAccess();", public_api)

        obsolete = (
            "pauseFeatureRequests",
            "activeFeatureLoops",
            "configUpdateInProgress",
            "pauseFeatures",
            "resumeFeatures",
            "isLoopFeaturesPaused",
            "beginFeatureLoop",
            "endFeatureLoop",
        )
        for name in obsolete:
            self.assertNotIn(name, self.header)
            self.assertNotIn(name, self.config)

    def test_config_update_writer_delegates_to_the_shared_lease(self) -> None:
        begin = block_after(
            self.config, r"bool\s+ConfigOnofre::tryBeginConfigUpdate\s*\(\s*\)"
        )
        self.assertEqual(begin.count("tryBeginFeatureAccess()"), 1)
        self.assertRegex(begin, r"return\s+tryBeginFeatureAccess\s*\(\s*\)\s*;")
        self.assertNotIn("featureAccessInProgress", begin)
        self.assertNotIn("#ifdef", begin)

        end = block_after(
            self.config, r"void\s+ConfigOnofre::endConfigUpdate\s*\(\s*\)"
        )
        self.assertEqual(end.count("endFeatureAccess()"), 1)
        self.assertRegex(
            end, r"endFeatureAccess\s*\(\s*\)\s*;"
        )
        self.assertNotIn("featureAccessInProgress", end)
        self.assertNotIn("#ifdef", end)

    def test_pin_arrays_are_strict_and_preserve_absent_fields(self) -> None:
        reader = block_after(
            self.config, r"PinArrayRead\s+readPinArrayStrict\s*\("
        )
        self.assertOrdered(
            reader,
            "value.isUnbound()",
            "return PinArrayRead::ABSENT;",
            "value.is<JsonArrayConst>()",
            "return PinArrayRead::INVALID;",
            "pin.is<unsigned int>()",
            "return PinArrayRead::INVALID;",
            "pin.as<unsigned int>()",
            "return PinArrayRead::VALID;",
        )
        self.assertNotIn("| 0u", reader)

    def test_preflight_enforces_shape_cardinality_and_unique_feature_entries(self) -> None:
        preflight = block_after(
            self.config, r"ConfigUpdateResult\s+preparePinUpdate\s*\("
        )
        self.assertIn("removeValue.is<JsonArrayConst>()", preflight)
        self.assertIn("item.is<const char *>()", preflight)
        self.assertIn("featuresValue.is<JsonArrayConst>()", preflight)
        self.assertIn("feature.is<JsonObjectConst>()", preflight)
        self.assertIn("if (plan->seen)", preflight)
        self.assertOrdered(
            preflight,
            "if (kind != FeatureKind::SENSOR)",
            "pins.size() != plan->oldInputs.size()",
            "Sensor::expectedInputCount(plan->sensorDriver)",
            "if (expectedInputs == 0)",
            "pins.size() != expectedInputs",
        )
        self.assertIn("pins.size() != plan->oldOutputs.size()", preflight)
        self.assertGreaterEqual(
            preflight.count("return ConfigUpdateResult::PIN_COUNT_MISMATCH;"), 2
        )

    def test_stored_duplicate_ids_are_rejected_before_ambiguous_lookup(self) -> None:
        preflight = block_after(
            self.config, r"ConfigUpdateResult\s+preparePinUpdate\s*\("
        )
        actuator_loop = block_after(
            preflight, r"for\s*\(\s*const\s+auto\s*&actuator\s*:\s*cfg\.actuatores\s*\)"
        )
        sensor_loop = block_after(
            preflight, r"for\s*\(\s*const\s+auto\s*&sensor\s*:\s*cfg\.sensors\s*\)"
        )
        for loop in (actuator_loop, sensor_loop):
            before_push = loop[: loop.index("plans.push_back(plan);")]
            self.assertIn("plans", before_push)
            self.assertRegex(
                before_push,
                r"equals\s*\(|find_if|count_if|duplicate|unique|already",
            )
            self.assertIn(
                "return ConfigUpdateResult::INVALID_REQUEST;", before_push
            )

    def test_final_state_validation_ignores_removed_features(self) -> None:
        preflight = block_after(
            self.config, r"ConfigUpdateResult\s+preparePinUpdate\s*\("
        )
        self.assertIn("if (!plan.removed)", preflight)
        self.assertIn("addClaims(plan, true, after);", preflight)
        self.assertIn("for (size_t i = 0; i < after.size(); i++)", preflight)
        self.assertIn("validPinForRole(cfg, after[i])", preflight)
        self.assertIn("for (size_t j = 0; j < after.size(); j++)", preflight)
        self.assertIn("after[i].pin == after[j].pin", preflight)
        self.assertNotIn("pinsAvailable", self.config)

    def test_only_exact_unchanged_claims_are_grandfathered(self) -> None:
        same_claim = block_after(self.config, r"bool\s+sameClaim\s*\(")
        for identity_field in (
            "left.kind == right.kind",
            "left.role == right.role",
            "left.slot == right.slot",
            "left.pin == right.pin",
            "left.id.equals(right.id)",
        ):
            self.assertIn(identity_field, same_claim)

        preflight = block_after(
            self.config, r"ConfigUpdateResult\s+preparePinUpdate\s*\("
        )
        self.assertOrdered(
            preflight,
            "if (containsClaim(before, after[i]))",
            "continue;",
            "if (!validPinForRole(cfg, after[i]))",
            "return ConfigUpdateResult::INVALID_PIN;",
            "after[i].pin == after[j].pin",
            "return ConfigUpdateResult::PIN_CONFLICT;",
        )

    def test_preflight_finishes_before_configuration_mutation(self) -> None:
        update = block_after(
            self.config, r"ConfigUpdateResult\s+ConfigOnofre::update\s*\("
        )
        self.assertOrdered(
            update,
            "if (!tryBeginConfigUpdate())",
            "return ConfigUpdateResult::BUSY;",
            "if (!restore)",
            "const ConfigUpdateResult result = preparePinUpdate",
            "if (result != ConfigUpdateResult::OK)",
            "endConfigUpdate();",
            "return result;",
            "releaseChangedOutputs(*this, pinPlans, oldClaims, newClaims);",
            'JsonVariantConst dhcpValue = root["dhcp"];',
            "if (!dhcpValue.isUnbound())",
            "dhcp = dhcpValue.as<bool>();",
        )
        self.assertOrdered(
            update,
            "setupMQTT(true);",
            "root.clear();",
            "if (!persist())",
            'responseRoot["restartRequired"] = true;',
            "return ConfigUpdateResult::PERSISTENCE_FAILED;",
            "json(responseRoot, true);",
            "endConfigUpdate();",
            "return ConfigUpdateResult::OK;",
        )

    def test_optional_scalars_are_validated_and_applied_as_a_patch(self) -> None:
        validator = block_after(
            self.config, r"bool\s+validateConfigScalarTypes\s*\("
        )
        self.assertIn('JsonVariantConst dhcpValue = root["dhcp"];', validator)
        self.assertIn("!dhcpValue.isUnbound()", validator)
        self.assertIn("!dhcpValue.is<bool>()", validator)
        self.assertIn('JsonVariantConst mqttPortValue = root["mqttPort"];', validator)
        self.assertIn("!mqttPortValue.is<unsigned int>()", validator)
        self.assertRegex(validator, r"port\s*==\s*0\s*\|\|\s*port\s*>\s*65535")

        update = block_after(
            self.config, r"ConfigUpdateResult\s+ConfigOnofre::update\s*\("
        )
        for field, target in (
            ("dhcp", "dhcp"),
            ("nodeId", "nodeId"),
            ("mqttPort", "mqttPort"),
            ("wifiSSID", "wifiSSID"),
        ):
            declaration = re.search(
                rf'JsonVariantConst\s+([A-Za-z_]\w*)\s*=\s*root\["{field}"\];',
                update,
            )
            self.assertIsNotNone(declaration, f"missing patch value for {field}")
            assert declaration is not None
            value_name = declaration.group(1)
            next_value = update.find("JsonVariantConst", declaration.end())
            field_apply = update[
                declaration.end() : next_value if next_value >= 0 else None
            ]
            self.assertOrdered(
                field_apply,
                f"if (!{value_name}.isUnbound())",
                target,
            )

    def test_legacy_backup_is_rejected_before_the_feature_lease(self) -> None:
        update = block_after(
            self.config, r"ConfigUpdateResult\s+ConfigOnofre::update\s*\("
        )
        self.assertOrdered(
            update,
            'JsonVariantConst backupValue = root["backup"];',
            "backupValue.as<bool>()",
            "return ConfigUpdateResult::INVALID_REQUEST;",
            "if (!tryBeginConfigUpdate())",
        )

    def test_atomic_json_persistence_never_deletes_the_known_good_target(self) -> None:
        persist = block_after(
            self.persistence, r"bool\s+persistJsonAtomically\s*\("
        )
        self.assertNotIn("LittleFS.remove(targetPath)", persist)
        self.assertOrdered(
            persist,
            "if (document.overflowed())",
            "return false;",
            "LittleFS.remove(temporaryPath);",
            'LittleFS.open(temporaryPath, "w")',
            "measureJson(document)",
            "serializeJson(document, temporary)",
            "temporary.flush();",
            "temporary.close();",
            "writtenBytes != expectedBytes",
            'LittleFS.open(temporaryPath, "r")',
            "verification.size()",
            "storedBytes != expectedBytes",
            "LittleFS.rename(temporaryPath, targetPath)",
        )

    def test_config_persistence_cannot_hide_per_actuator_json_overflow(self) -> None:
        config_persist = block_after(
            self.config, r"bool\s+ConfigOnofre::persist\s*\("
        )
        actuator_loop = block_after(
            config_persist,
            r"for\s*\(\s*(?:const\s+)?auto(?:\s*&)?\s+s\s*:\s*actuatores\s*\)",
        )

        if re.search(r"JsonDocument\s+a\s*;", actuator_loop):
            self.assertOrdered(
                actuator_loop,
                "a.overflowed()",
                "return false;",
                "features.add(a);",
            )
        else:
            self.assertRegex(
                actuator_loop,
                r"JsonObject\s+a\s*=\s*features\.add<JsonObject>\s*\(\s*\)\s*;",
            )

    def test_config_and_irrigation_use_separate_atomic_temp_files(self) -> None:
        config_persist = block_after(
            self.config, r"bool\s+ConfigOnofre::persist\s*\("
        )
        irrigation_save = block_after(
            self.irrigation, r"bool\s+Irrigation::save\s*\("
        )
        self.assertRegex(
            config_persist,
            r"persistJsonAtomically\s*\(\s*configFilenames::config\s*,\s*"
            r"configFilenames::configTemporary\s*,\s*doc\s*\)",
        )
        self.assertRegex(
            irrigation_save,
            r"persistJsonAtomically\s*\(\s*configFilenames::irrigation\s*,\s*"
            r"configFilenames::irrigationTemporary\s*,\s*doc\s*\)",
        )

    def test_schedule_replacement_closes_the_old_active_zone_first(self) -> None:
        update = block_after(
            self.irrigation, r"bool\s+Irrigation::update\s*\("
        )
        self.assertOrdered(
            update,
            "if (isRunning())",
            "stop();",
            "programs = parsed;",
        )

    def test_irrigation_replacement_requires_complete_top_level_shape(self) -> None:
        _, validation = self.irrigationValidationPrefix()

        enabled = re.search(
            r'JsonVariantConst\s+(\w+)\s*=\s*root\["enabled"\]\s*;',
            validation,
        )
        skip_on_rain = re.search(
            r'JsonVariantConst\s+(\w+)\s*=\s*root\["skipOnRain"\]\s*;',
            validation,
        )
        programs = re.search(
            r'JsonVariantConst\s+(\w+)\s*=\s*root\["programs"\]\s*;',
            validation,
        )
        self.assertIsNotNone(enabled)
        self.assertIsNotNone(skip_on_rain)
        self.assertIsNotNone(programs)
        assert enabled is not None and skip_on_rain is not None and programs is not None

        self.assertRegex(validation, rf"{enabled.group(1)}\.is<bool>\s*\(\s*\)")
        self.assertRegex(validation, rf"{skip_on_rain.group(1)}\.is<bool>\s*\(\s*\)")
        self.assertRegex(
            validation,
            rf"{programs.group(1)}\.is<JsonArray(?:Const)?>\s*\(\s*\)",
        )
        program_list = re.search(
            rf"JsonArray(?:Const)?\s+(\w+)\s*=\s*{programs.group(1)}"
            rf"\.as<JsonArray(?:Const)?>\s*\(\s*\)\s*;",
            validation,
        )
        if program_list is None:
            size_source = programs.group(1)
        else:
            size_source = program_list.group(1)
        self.assertRegex(
            validation,
            rf"{size_source}\.size\s*\(\s*\)\s*>\s*maxPrograms",
        )
        self.assertIn("return false;", validation)

    def test_irrigation_programs_are_strict_and_ids_are_unique(self) -> None:
        _, validation = self.irrigationValidationPrefix()

        program_value = re.search(
            r"for\s*\(\s*JsonVariant(?:Const)?\s+(\w+)\s*:", validation
        )
        self.assertIsNotNone(program_value)
        assert program_value is not None
        item = program_value.group(1)
        self.assertRegex(validation, rf"{item}\.is<JsonObject(?:Const)?>\s*\(")
        program_object = re.search(
            rf"JsonObject(?:Const)?\s+(\w+)\s*=\s*{item}\.as<JsonObject(?:Const)?>\s*\(",
            validation,
        )
        self.assertIsNotNone(program_object)
        assert program_object is not None
        program = program_object.group(1)

        field_types = {
            "id": r"unsigned int",
            "enabled": r"bool",
            "startMinute": r"unsigned int",
            "weekdays": r"unsigned int",
            "zones": r"JsonArray(?:Const)?",
        }
        values: dict[str, str] = {}
        for field, expected_type in field_types.items():
            declaration = re.search(
                rf'JsonVariantConst\s+(\w+)\s*=\s*{program}\["{field}"\]\s*;',
                validation,
            )
            self.assertIsNotNone(declaration, f"missing required program field {field}")
            assert declaration is not None
            values[field] = declaration.group(1)
            self.assertRegex(
                validation,
                rf"{declaration.group(1)}\.is<{expected_type}>\s*\(\s*\)",
            )

        id_number = re.search(
            rf"(?:const\s+)?unsigned int\s+(\w+)\s*=\s*"
            rf"{values['id']}\.as<unsigned int>\s*\(\s*\)\s*;",
            validation,
        )
        self.assertIsNotNone(id_number)
        assert id_number is not None
        program_id = id_number.group(1)
        self.assertRegex(validation, rf"{program_id}\s*==\s*0")
        self.assertRegex(validation, rf"{program_id}\s*>\s*255")

        start_number = re.search(
            rf"(?:const\s+)?unsigned int\s+(\w+)\s*=\s*"
            rf"{values['startMinute']}\.as<unsigned int>\s*\(\s*\)\s*;",
            validation,
        )
        weekdays_number = re.search(
            rf"(?:const\s+)?unsigned int\s+(\w+)\s*=\s*"
            rf"{values['weekdays']}\.as<unsigned int>\s*\(\s*\)\s*;",
            validation,
        )
        self.assertIsNotNone(start_number)
        self.assertIsNotNone(weekdays_number)
        assert start_number is not None and weekdays_number is not None
        self.assertRegex(validation, rf"{start_number.group(1)}\s*>\s*1439")
        self.assertRegex(validation, rf"{weekdays_number.group(1)}\s*>\s*(?:127|0x7F)")

        self.assertRegex(
            validation,
            r"(?:std::find_if\s*\(\s*parsed\.begin\s*\(\s*\)\s*,\s*"
            r"parsed\.end\s*\(\s*\)|"
            r"for\s*\(\s*const\s+auto\s*&\s*\w+\s*:\s*parsed\s*\))",
        )
        self.assertRegex(
            validation,
            rf"if\s*\(\s*\w+\.id\s*==\s*{program_id}\s*\)\s*"
            rf"(?:\{{\s*)?return\s+false\s*;",
        )

    def test_irrigation_zones_are_strict_unique_and_bounded(self) -> None:
        _, validation = self.irrigationValidationPrefix()

        zones_value = re.search(
            r'JsonVariantConst\s+(\w+)\s*=\s*\w+\["zones"\]\s*;',
            validation,
        )
        self.assertIsNotNone(zones_value)
        assert zones_value is not None
        zones_array = re.search(
            rf"JsonArray(?:Const)?\s+(\w+)\s*=\s*{re.escape(zones_value.group(1))}"
            rf"\.as<JsonArray(?:Const)?>\s*\(\s*\)\s*;",
            validation,
        )
        if zones_array is None:
            zones_source = (
                rf"{re.escape(zones_value.group(1))}\.as<JsonArray(?:Const)?>"
                r"\s*\(\s*\)"
            )
        else:
            zones_source = re.escape(zones_array.group(1))
        zone_value = re.search(
            rf"for\s*\(\s*JsonVariant(?:Const)?\s+(\w+)\s*:\s*"
            rf"{zones_source}\s*\)",
            validation,
        )
        self.assertIsNotNone(zone_value)
        assert zone_value is not None
        item = zone_value.group(1)
        self.assertRegex(validation, rf"{item}\.is<JsonObject(?:Const)?>\s*\(")
        zone_object = re.search(
            rf"JsonObject(?:Const)?\s+(\w+)\s*=\s*{item}\.as<JsonObject(?:Const)?>\s*\(",
            validation,
        )
        self.assertIsNotNone(zone_object)
        assert zone_object is not None
        zone = zone_object.group(1)

        unique_id = re.search(
            rf'JsonVariantConst\s+(\w+)\s*=\s*{zone}\["uniqueId"\]\s*;',
            validation,
        )
        minutes = re.search(
            rf'JsonVariantConst\s+(\w+)\s*=\s*{zone}\["minutes"\]\s*;',
            validation,
        )
        self.assertIsNotNone(unique_id)
        self.assertIsNotNone(minutes)
        assert unique_id is not None and minutes is not None
        self.assertRegex(
            validation, rf"{unique_id.group(1)}\.is<const char\s*\*>\s*\(\s*\)"
        )
        self.assertRegex(
            validation, rf"{minutes.group(1)}\.is<unsigned int>\s*\(\s*\)"
        )

        id_string = re.search(
            rf"const char\s*\*\s*(\w+)\s*=\s*"
            rf"{unique_id.group(1)}\.as<const char\s*\*>\s*\(\s*\)\s*;",
            validation,
        )
        minute_number = re.search(
            rf"(?:const\s+)?unsigned int\s+(\w+)\s*=\s*"
            rf"{minutes.group(1)}\.as<unsigned int>\s*\(\s*\)\s*;",
            validation,
        )
        self.assertIsNotNone(id_string)
        self.assertIsNotNone(minute_number)
        assert id_string is not None and minute_number is not None
        zone_id = id_string.group(1)
        zone_minutes = minute_number.group(1)
        self.assertRegex(
            validation,
            rf"(?:!\s*{zone_id}\s*\[\s*0\s*\]|"
            rf"{zone_id}\s*\[\s*0\s*\]\s*==\s*'\\0')",
        )
        self.assertRegex(
            validation,
            rf"strlen\s*\(\s*{zone_id}\s*\)\s*>=\s*sizeof\s*\([^)]*uniqueId[^)]*\)",
        )
        self.assertRegex(validation, rf"{zone_minutes}\s*==\s*0")
        self.assertRegex(validation, rf"{zone_minutes}\s*>\s*maxZoneMinutes")

        self.assertRegex(
            validation,
            r"(?:std::find_if\s*\(\s*\w+\.zones\.begin\s*\(\s*\)\s*,\s*"
            r"\w+\.zones\.end\s*\(\s*\)|"
            rf"for\s*\(\s*JsonVariant(?:Const)?\s+\w+\s*:\s*{zones_source}\s*\))",
        )
        self.assertRegex(
            validation,
            rf"if\s*\(\s*strcmp\s*\([^,]+\s*,\s*{zone_id}\s*\)\s*"
            rf"==\s*0\s*\)\s*(?:\{{\s*)?return\s+false\s*;",
        )
        self.assertRegex(
            validation,
            rf"if\s*\(\s*!findZone\s*\(\s*{zone_id}\s*\)\s*\)\s*continue\s*;",
        )

        # Strict replacement rejects invalid numbers rather than silently
        # clamping or masking them into a different schedule.
        update, _ = self.irrigationValidationPrefix()
        self.assertNotRegex(update, r"\b(?:min|max)\s*\(")
        self.assertNotRegex(update, r"&\s*0x7F")

    def test_garden_valves_are_forced_off_at_boot_and_when_persisted(self) -> None:
        setup = block_after(self.actuator, r"void\s+Actuator::setup\s*\(\s*\)")
        self.assertOrdered(
            setup,
            "if (isGardenValve())",
            "state = ActuatorState::OFF_OPEN;",
            "configPIN(output, OUTPUT);",
            "writeToPIN(output, state);",
        )

        persist = block_after(self.config, r"bool\s+ConfigOnofre::persist\s*\(")
        self.assertIn(
            'a["state"] = s.isGardenValve() ? ActuatorState::OFF_OPEN : s.state;',
            persist,
        )

    def test_config_persistence_failure_hands_the_lease_to_restart(self) -> None:
        update = block_after(
            self.config, r"ConfigUpdateResult\s+ConfigOnofre::update\s*\("
        )
        failed = block_after(update, r"if\s*\(\s*!persist\(\)\s*\)")
        self.assertIn('responseRoot["restartRequired"] = true;', failed)
        self.assertIn("return ConfigUpdateResult::PERSISTENCE_FAILED;", failed)
        self.assertNotIn("endConfigUpdate();", failed)

    def test_interactive_persistence_failures_return_507_and_restart(self) -> None:
        feature_start = self.server.index("/*CREATE NEW FEATURE*/")
        feature_end = self.server.index("/*CONTROL ACTUATOR*/", feature_start)
        feature_handler = self.server[feature_start:feature_end]
        feature_failure = block_after(
            feature_handler, r"if\s*\(\s*!config\.persist\(\)\s*\)"
        )
        self.assertIn("ConfigUpdateResult::PERSISTENCE_FAILED", feature_failure)
        self.assertIn("response->setCode(507);", feature_failure)
        self.assertIn('response->addHeader("Connection", "close");', feature_failure)
        self.assertIn("request->onDisconnect", feature_failure)
        self.assertIn("config.requestRestart();", feature_failure)
        self.assertNotIn("config.endFeatureAccess();", feature_failure)

        irrigation_start = self.server.index("/*IRRIGATION SCHEDULE*/")
        irrigation_end = self.server.index('"/irrigation-run"', irrigation_start)
        irrigation_handler = self.server[irrigation_start:irrigation_end]
        irrigation_failure = block_after(
            irrigation_handler, r"else\s+if\s*\(\s*!irrigation\.save\(\)\s*\)"
        )
        self.assertIn("response->setCode(507);", irrigation_failure)
        self.assertIn('response->addHeader("Connection", "close");', irrigation_failure)
        self.assertIn("request->onDisconnect", irrigation_failure)
        self.assertIn("config.requestRestart();", irrigation_failure)
        self.assertNotIn("config.endFeatureAccess();", irrigation_failure)

        captive = block_after(
            self.server,
            r"void\s+handleRequest\s*\(\s*AsyncWebServerRequest\s*\*request\s*\)",
        )
        store = block_after(captive, r"if\s*\(\s*store\s*\)")
        self.assertIn("if (config.persist())", store)
        self.assertIn("response->setCode(507);", store)
        self.assertIn('response->addHeader("Connection", "close");', store)
        self.assertIn("request->onDisconnect", store)
        self.assertIn("config.requestRestart();", store)
        self.assertNotIn("config.endFeatureAccess();", store)

    def test_diagnostic_snapshot_cannot_submit_a_restore(self) -> None:
        export = block_after(self.panel, r"function\s+exportConfig\s*\(")
        restore = block_after(self.panel, r"function\s+restoreConfig\s*\(")
        self.assertIn('snapshotType: "easyiot-diagnostic"', export)
        self.assertNotIn("backup: true", export)
        self.assertNotIn("api(", restore)
        self.assertNotIn("fetch(", restore)
        self.assertIn("restauro está desativado", restore)
        self.assertRegex(self.panel_html, r'id="r-file"[^>]*\bdisabled\b')
        self.assertRegex(self.panel_html, r'id="r-send"[^>]*\bdisabled\b')

    def test_changed_outputs_are_parked_without_driving_new_inputs(self) -> None:
        park = block_after(self.config, r"void\s+parkOutput\s*\(")
        low_block = block_after(park, r"if\s*\(\s*!remainsAnInput\s*\)")
        self.assertOrdered(
            low_block,
            "configPIN(pin, OUTPUT);",
            "writeToPIN(pin, 0);",
        )
        self.assertEqual(park.count("writeToPIN(pin, 0);"), 1)
        self.assertGreater(
            park.find("configPIN(pin, INPUT);"),
            park.find("writeToPIN(pin, 0);"),
        )

        release = block_after(
            self.config, r"void\s+releaseChangedOutputs\s*\("
        )
        unchanged_output = block_after(
            self.config, r"bool\s+pinHasUnchangedOutput\s*\("
        )
        self.assertIn("claim.role == PinRole::OUTPUT_PIN", unchanged_output)
        self.assertIn("containsClaim(before, claim)", unchanged_output)
        self.assertIn("return true;", unchanged_output)
        self.assertIn("plan.oldOutputs", release)
        self.assertNotIn("plan.oldInputs", release)
        self.assertIn("containsClaim(after, oldClaim)", release)
        # Only sameClaim(id, kind, role, slot, pin) may grandfather an output.
        # A different final output owner is a real transfer: it must be parked
        # before the new feature's setup claims the GPIO.
        self.assertNotIn(
            "pinHasRole(after, oldClaim.pin, PinRole::OUTPUT_PIN)", release
        )
        self.assertIn(
            "pinHasUnchangedOutput(before, after, oldClaim.pin)", release
        )
        # Grandfathered historical maps can contain pins this firmware would no
        # longer accept. Never touch such a GPIO during release.
        self.assertIn("!cfg.validOutputPin(oldClaim.pin)", release)
        self.assertIn("pinAlreadyReleased(released, oldClaim.pin)", release)
        self.assertIn("pinHasRole(after, oldClaim.pin, PinRole::INPUT_PIN)", release)
        self.assertIn("parkOutput(oldClaim.pin, remainsAnInput);", release)

    def test_config_api_maps_rejection_and_busy_to_http_errors(self) -> None:
        start = self.server.index("/*SAVE CONFIG*/")
        end = self.server.index("/*CREATE NEW FEATURE*/", start)
        handler = self.server[start:end]

        self.assertIn("json.is<JsonObject>()", handler)
        self.assertIn("ConfigUpdateResult::INVALID_REQUEST", handler)
        self.assertRegex(handler, r"(?:const\s+)?ConfigUpdateResult\s+result")
        self.assertRegex(
            handler,
            r"(?:result\s*=\s*|ConfigUpdateResult\s+result\s*=\s*)"
            r"config\.update\s*\(\s*configJson\s*,\s*root\s*\)",
        )
        self.assertIn("if (result != ConfigUpdateResult::OK)", handler)
        self.assertRegex(
            handler,
            r'root\["result"\]\s*=\s*static_cast<int>\s*\(\s*result\s*\)',
        )
        self.assertRegex(
            handler,
            r"ConfigUpdateResult::BUSY[\s\S]{0,240}(?:setCode\s*\(\s*409\s*\)|\?\s*409)",
        )
        self.assertRegex(
            handler,
            r"ConfigUpdateResult::PERSISTENCE_FAILED[\s\S]{0,160}setCode\s*\(\s*507\s*\)",
        )
        self.assertRegex(handler, r"setCode\s*\([^;]*400")
        self.assertOrdered(
            handler,
            'root["restartRequired"]',
            'response->addHeader("Connection", "close")',
            "request->onDisconnect",
            "config.requestRestart();",
            "request->send(response);",
        )
        self.assertIn("response->setLength();", handler)
        self.assertIn("request->send(response);", handler)
        # The transaction owns its quiescence. Pausing here would make its
        # zero-owner compare/exchange reject its own request as BUSY.
        self.assertNotIn("pauseFeatures", handler)
        self.assertNotIn("resumeFeatures", handler)

    def test_webui_explains_every_config_rejection_and_keeps_save_dirty(self) -> None:
        api = block_after(self.panel, r"async\s+function\s+api\s*\(")
        self.assertIn("err.body = body;", api)

        error_mapper = block_after(
            self.panel, r"function\s+configError\s*\(\s*e\s*\)"
        )
        mappings = {
            int(code): message
            for code, message in re.findall(
                r'if\s*\(\s*code\s*===\s*(\d+)\s*\)\s*return\s*"([^"]+)"\s*;',
                error_mapper,
            )
        }
        self.assertEqual(set(mappings), {1, 2, 3, 4, 5, 6})
        self.assertRegex(mappings[1].lower(), r"inválid|recus")
        self.assertRegex(mappings[2].lower(), r"gpio|pino")
        self.assertRegex(mappings[3].lower(), r"pino")
        self.assertRegex(mappings[4].lower(), r"usad|ocupad|conflit")
        self.assertRegex(mappings[5].lower(), r"curso|tenta|novamente|ocupad")
        self.assertRegex(mappings[6].lower(), r"memória|guard|configuração anterior")

        save = block_after(self.panel, r"async\s+function\s+save\s*\(\s*\)")
        self.assertIn('toast(configError(e), "err");', save)
        self.assertIn("saved.restartRequired === true", save)
        self.assertRegex(save, r'Guardado[^"\n]*reiniciar')
        self.assertEqual(save.count("clearDirty();"), 1)
        self.assertLess(save.find("clearDirty();"), save.find("catch (e)"))

    def test_webui_preserves_single_cover_input_topology(self) -> None:
        modes = block_after(self.panel, r"function\s+inputModes\s*\(\s*f\s*\)")
        self.assertIn('driver === "COVER_SINGLE_PUSH"', modes)
        self.assertIn("COVER_SINGLE_MODE", modes)
        self.assertIn('driver.indexOf("COVER_DUAL")', modes)
        self.assertIn("return INPUT_MODES;", modes)
        self.assertRegex(modes, r'GARAGE_PUSH[^\n]*GARDEN_VALVE')
        self.assertIn("return [];", modes)

        self.assertRegex(
            self.panel,
            r"COVER_SINGLE_MODE\s*=\s*\{[^}]*v\s*:\s*2",
        )

    def test_ota_acquires_the_feature_lease_only_for_the_first_upload_chunk(self) -> None:
        upload = block_after(
            self.server,
            r"\[\]\s*\(\s*AsyncWebServerRequest\s*\*request\s*,\s*String\s+filename\s*,\s*"
            r"size_t\s+index\s*,\s*uint8_t\s*\*data\s*,\s*size_t\s+len\s*,\s*bool\s+final\s*\)",
        )
        self.assertEqual(upload.count("config.tryBeginFeatureAccess()"), 1)
        first_chunk = block_after(upload, r"if\s*\(\s*!index\s*\)")
        self.assertIn("config.tryBeginFeatureAccess()", first_chunk)
        self.assertIn("authorizeRequest(request, false, &authBusy)", first_chunk)
        self.assertIn("state->busy = true;", first_chunk)
        self.assertIn("state->ownsFeatureAccess = true;", first_chunk)
        self.assertNotIn("requestAuthentication", first_chunk)
        before_first_chunk = upload[: upload.index("if (!index)")]
        self.assertNotIn("config.tryBeginFeatureAccess()", before_first_chunk)
        self.assertNotIn("authorizeRequest(request)", before_first_chunk)
        self.assertNotIn("pauseFeatures", upload)
        self.assertNotIn("resumeFeatures", upload)

    def test_ota_network_error_never_claims_success(self) -> None:
        firmware = block_after(self.panel, r"function\s+uploadFirmware\s*\(")
        self.assertIn("xhr.onload = () => done(xhr.status === 200);", firmware)
        self.assertIn("xhr.onerror = () => done(false);", firmware)
        self.assertNotRegex(firmware, r"xhr\.onerror[^\n]*fill\.style\.width")

    def test_auto_update_starts_after_the_http_response_disconnects(self) -> None:
        handler = block_after(
            self.server,
            r"autoUpdateHandler\s*=\s*\[\]\s*\(\s*AsyncWebServerRequest\s*\*request",
        )
        self.assertOrdered(
            handler,
            'response->addHeader("Connection", "close");',
            "request->onDisconnect",
            "config.requestAutoUpdate();",
            "request->send(response);",
        )

    def test_auto_update_polling_never_treats_connection_loss_as_success(self) -> None:
        follow = block_after(self.panel, r"function\s+followUpdate\s*\(")

        self.assertIn('const startingVersion = String(config.firmware || "");', follow)
        self.assertIn("snapshot.firmware !== startingVersion", follow)
        self.assertIn("Date.now() >= deadline", follow)
        self.assertNotRegex(follow, r"miss(?:es)?\s*>?=\s*\d+")
        catch = block_after(follow, r"catch\s*\(\s*e\s*\)")
        self.assertNotIn("finish(true", catch)

    def test_irrigation_action_routes_match_between_firmware_and_panel(self) -> None:
        for route in ("/irrigation-run", "/irrigation-stop"):
            self.assertIn(f'"{route}"', self.server)
            self.assertIn(f'"{route}"', self.panel)

        # v9.186 keeps slash-path aliases for panels that were already open
        # during an OTA. They must be registered before the prefix-matching
        # schedule endpoint and share the same hardened callbacks.
        schedule = self.server.index(
            'new AsyncCallbackJsonWebHandler("/irrigation",'
        )
        legacy_run = self.server.index(
            'new AsyncCallbackJsonWebHandler("/irrigation/run", irrigationRunHandler)'
        )
        legacy_stop = self.server.index(
            'server.on("/irrigation/stop", HTTP_POST, irrigationStopHandler)'
        )
        self.assertLess(legacy_run, schedule)
        self.assertLess(legacy_stop, schedule)
        self.assertIn(
            'new AsyncCallbackJsonWebHandler("/irrigation-run", irrigationRunHandler)',
            self.server,
        )
        self.assertIn(
            'server.on("/irrigation-stop", HTTP_POST, irrigationStopHandler)',
            self.server,
        )
        self.assertNotIn('"/irrigation/run"', self.panel)
        self.assertNotIn('"/irrigation/stop"', self.panel)

    def test_absent_input_mode_preserves_the_preflighted_driver(self) -> None:
        plan = block_after(self.config, r"struct\s+FeaturePinPlan")
        self.assertRegex(
            plan,
            r"ActuatorDriver\s+driver\s*=\s*(?:ActuatorDriver::)?INVALID\s*;",
        )

        preflight = block_after(
            self.config, r"ConfigUpdateResult\s+preparePinUpdate\s*\("
        )
        actuator_plan = block_after(
            preflight,
            r"for\s*\(\s*const\s+auto\s*&actuator\s*:\s*cfg\.actuatores\s*\)",
        )
        self.assertIn("plan.driver = actuator.driver;", actuator_plan)

        actuator_update = block_after(
            preflight, r"if\s*\(\s*kind\s*==\s*FeatureKind::ACTUATOR\s*\)"
        )
        self.assertIn('feature["inputMode"]', actuator_update)
        self.assertIn("isUnbound()", actuator_update)
        self.assertIn("is<unsigned int>()", actuator_update)
        self.assertIn("as<unsigned int>()", actuator_update)

        # The assignment must be inside the provided-value branch. Initialising
        # the plan from actuator.driver above is what makes an omitted field a
        # true partial update rather than an implicit switch to PUSH.
        provided = re.search(
            r"if\s*\(\s*!\s*([A-Za-z_]\w*)\.isUnbound\(\)\s*\)",
            actuator_update,
        )
        self.assertIsNotNone(provided)
        assert provided is not None
        provided_block = block_after(
            actuator_update,
            rf"if\s*\(\s*!\s*{re.escape(provided.group(1))}\.isUnbound\(\)\s*\)",
        )
        self.assertIn("plan->driver", provided_block)
        self.assertRegex(provided_block, r"findDriver\s*\(")

    def test_input_mode_is_family_validated_and_cover_cardinality_is_safe(self) -> None:
        preflight = block_after(
            self.config, r"ConfigUpdateResult\s+preparePinUpdate\s*\("
        )
        actuator_update = block_after(
            preflight, r"if\s*\(\s*kind\s*==\s*FeatureKind::ACTUATOR\s*\)"
        )

        # Strict means JSON strings, negative values, and unknown enum values
        # are rejected before any cast or findDriver() call.
        self.assertOrdered(
            actuator_update,
            "is<unsigned int>()",
            "return ConfigUpdateResult::INVALID_REQUEST;",
            "as<unsigned int>()",
        )
        self.assertRegex(
            actuator_update,
            r"(?:>|>=)\s*(?:static_cast<unsigned int>\s*\(\s*)?"
            r"ActuatorInputMode::ROTATE",
        )

        # Accept either a small named helper or an inline family guard, but it
        # must distinguish actuator families and reject unsupported modes.
        helper_match = re.search(
            r"bool\s+([A-Za-z_]\w*(?:(?:allow|support|valid)[A-Za-z_]*"
            r"InputMode|InputMode[A-Za-z_]*(?:allow|support|valid))[A-Za-z_]*)\s*\(",
            self.config,
            flags=re.IGNORECASE,
        )
        if helper_match is not None:
            helper_name = helper_match.group(1)
            helper = block_after(
                self.config, rf"bool\s+{re.escape(helper_name)}\s*\("
            )
            self.assertRegex(
                helper,
                r"isCover\s*\(|isLight\s*\(|isSwitch\s*\(|isGarage\s*\(|"
                r"isGardenValve\s*\(|COVER_|LIGHT_|SWITCH_|GARAGE_|GARDEN_",
            )
            self.assertIn(helper_name, actuator_update)
            guard_start = actuator_update.index(helper_name)
            self.assertIn(
                "return ConfigUpdateResult::INVALID_REQUEST;",
                actuator_update[guard_start:],
            )
        else:
            self.assertRegex(
                actuator_update,
                r"isCover\s*\(|isLight\s*\(|isSwitch\s*\(|isGarage\s*\(|"
                r"isGardenValve\s*\(|COVER_|LIGHT_|SWITCH_|GARAGE_|GARDEN_",
            )

        # Changing a cover between one-button and two-button drivers without
        # changing the input array shape must be refused before apply.
        cardinality_source = block_after(
            self.config,
            r"ConfigUpdateResult\s+validateActuatorTopology\s*\(",
        )
        self.assertIn("ActuatorDriver::COVER_SINGLE_PUSH", cardinality_source)
        self.assertIn("ActuatorDriver::COVER_DUAL_PUSH", cardinality_source)
        self.assertIn("ActuatorDriver::COVER_DUAL_LATCH", cardinality_source)
        self.assertRegex(
            cardinality_source,
            r"(?:inputs\.size\(\)|inputCount)\s*(?:!=|==)\s*1",
        )
        self.assertRegex(
            cardinality_source,
            r"(?:inputs\.size\(\)|inputCount)\s*(?:!=|==)\s*2",
        )
        self.assertIn(
            "return ConfigUpdateResult::PIN_COUNT_MISMATCH;",
            cardinality_source,
        )

    def test_apply_uses_the_driver_accepted_by_preflight(self) -> None:
        update = block_after(
            self.config, r"ConfigUpdateResult\s+ConfigOnofre::update\s*\("
        )
        feature_loop = block_after(
            update, r"for\s*\(\s*auto\s+feature\s*:\s*features\s*\)"
        )
        actuator_group = block_after(
            feature_loop, r"if\s*\(\s*String\(\"ACTUATOR\"\)\.equals"
        )
        normal_start = actuator_group.find("else", actuator_group.find("if (restore)"))
        self.assertGreaterEqual(normal_start, 0)
        normal_update = actuator_group[normal_start:]

        self.assertOrdered(
            normal_update,
            "FeaturePinPlan *plan = findPlan",
            "actuator.driver = plan->driver;",
        )
        self.assertNotRegex(
            normal_update,
            r"actuator\.findDriver\s*\(\s*feature\s*\[\s*\"inputMode\"\s*\]",
        )

    def test_input_handler_rebuild_does_not_touch_outputs_or_knx(self) -> None:
        self.assertIn("void rebuildInputHandlers();", self.actuator_header)
        rebuild = block_after(
            self.actuator, r"void\s+Actuator::rebuildInputHandlers\s*\(\s*\)"
        )
        self.assertIn("buttons.clear();", rebuild)
        self.assertNotIn("new Shutters", rebuild)
        self.assertNotIn("writeToPIN", rebuild)
        self.assertNotRegex(rebuild, r"configPIN\s*\([^,]+,\s*OUTPUT\s*\)")
        self.assertNotIn("knx.callback_assign", rebuild)

    def test_garden_valve_wall_button_uses_the_pressed_toggle_handler(self) -> None:
        rebuild = block_after(
            self.actuator, r"void\s+Actuator::rebuildInputHandlers\s*\(\s*\)"
        )
        self.assertRegex(
            rebuild,
            r"if\s*\(\s*isLight\(\)\s*\|\|\s*isSwitch\(\)\s*\|\|\s*"
            r"isGardenValve\(\)\s*\)",
        )
        driver_switch = block_after(rebuild, r"switch\s*\(\s*driver\s*\)")
        self.assertOrdered(
            driver_switch,
            "case ActuatorDriver::GARDEN_VALVE:",
            "button.setPressedHandler(toogle);",
            "break;",
        )

    def test_metadata_and_input_only_updates_avoid_full_actuator_setup(self) -> None:
        update = block_after(
            self.config, r"ConfigUpdateResult\s+ConfigOnofre::update\s*\("
        )
        preflight = block_after(
            self.config, r"ConfigUpdateResult\s+preparePinUpdate\s*\("
        )
        feature_loop = block_after(
            update, r"for\s*\(\s*auto\s+feature\s*:\s*features\s*\)"
        )
        actuator_group = block_after(
            feature_loop, r"if\s*\(\s*String\(\"ACTUATOR\"\)\.equals"
        )
        normal_start = actuator_group.find("else", actuator_group.find("if (restore)"))
        self.assertGreaterEqual(normal_start, 0)
        normal_update = actuator_group[normal_start:]

        def changed_flag(fragment: str) -> str:
            match = re.search(
                rf"const\s+bool\s+([A-Za-z_]\w*{fragment}[A-Za-z_]*Changed|"
                rf"[A-Za-z_]\w*Changed[A-Za-z_]*{fragment}[A-Za-z_]*|"
                rf"{fragment}s?Changed)\s*=",
                normal_update,
                flags=re.IGNORECASE,
            )
            self.assertIsNotNone(match, f"missing {fragment} changed flag")
            assert match is not None
            return match.group(1)

        inputs_changed = changed_flag("input")
        driver_changed = changed_flag("driver")

        # Hardware-level restart classification belongs in preflight so the
        # old actuator can be made inert and its outputs handed off before any
        # live configuration is mutated. Do not duplicate this policy with
        # apply-local output/course/KNX flags.
        planned_restart = re.search(
            r"plan->restartRequired\s*=\s*([^;]+);",
            preflight,
            flags=re.DOTALL,
        )
        self.assertIsNotNone(planned_restart, "missing preflight restart decision")
        assert planned_restart is not None
        planned_expr = planned_restart.group(1)
        for required in (
            "plan->oldOutputs != plan->outputs",
            "actuator->upCourseTime != nextUpCourseTime",
            "actuator->downCourseTime != nextDownCourseTime",
            "actuator->knxAddress[0] != nextKnxArea",
            "actuator->knxAddress[1] != nextKnxLine",
            "actuator->knxAddress[2] != nextKnxMember",
        ):
            self.assertIn(required, planned_expr)
        self.assertNotIn("oldInputs", planned_expr)
        self.assertNotIn("driver", planned_expr)

        restart = re.search(
            r"const\s+bool\s+((?=\w*restart)(?=\w*required)[A-Za-z_]\w*)"
            r"\s*=\s*([^;]+);",
            normal_update,
            flags=re.IGNORECASE,
        )
        self.assertIsNotNone(restart, "missing explicit controlled-restart decision")
        assert restart is not None
        restart_name, restart_expr = restart.groups()
        self.assertIn("plan->restartRequired", restart_expr)
        for forbidden in (inputs_changed, driver_changed):
            self.assertNotIn(forbidden, restart_expr)

        restart_branch = block_after(
            normal_update,
            rf"if\s*\(\s*{re.escape(restart_name)}\s*\)",
        )
        self.assertIn("restartRequired = true;", restart_branch)

        rebuild_call = normal_update.index("actuator.rebuildInputHandlers();")
        self.assertGreater(rebuild_call, normal_update.index(inputs_changed))
        self.assertGreater(rebuild_call, normal_update.index(driver_changed))

        rebuild_decision = re.search(
            rf"const\s+bool\s+([A-Za-z_]\w*)\s*=\s*([^;]*"
            rf"(?:{re.escape(inputs_changed)}[^;]*{re.escape(driver_changed)}|"
            rf"{re.escape(driver_changed)}[^;]*{re.escape(inputs_changed)})[^;]*);",
            normal_update,
        )
        if rebuild_decision is not None:
            rebuild_branch = block_after(
                normal_update,
                rf"(?:else\s+)?if\s*\(\s*"
                rf"{re.escape(rebuild_decision.group(1))}\s*\)",
            )
        else:
            rebuild_branch = block_after(
                normal_update,
                rf"(?:else\s+)?if\s*\([^)]*(?:"
                rf"{re.escape(inputs_changed)}[^)]*{re.escape(driver_changed)}|"
                rf"{re.escape(driver_changed)}[^)]*{re.escape(inputs_changed)})[^)]*\)",
            )
        self.assertIn("actuator.rebuildInputHandlers();", rebuild_branch)

        # Full setup is never safe inside this request: it can pulse outputs,
        # duplicate KNX callbacks, and recreate the shutter controller. A
        # hardware-level change is persisted and applied by controlled reboot.
        self.assertEqual(normal_update.count("actuator.setup();"), 0)
        self.assertEqual(normal_update.count("actuator.rebuildInputHandlers();"), 1)

        self.assertOrdered(
            update,
            "preparePinUpdate",
            "deactivateForConfigUpdate();",
            "releaseChangedOutputs",
        )
        self.assertIn('responseRoot["restartRequired"] = true;', update)
        self.assertNotIn("requestRestart();", update)

    def test_network_identity_changes_require_response_then_restart(self) -> None:
        update = block_after(
            self.config, r"ConfigUpdateResult\s+ConfigOnofre::update\s*\("
        )
        for snapshot in (
            "previousNodeId",
            "previousWifiSSID",
            "previousWifiSecret",
            "previousWifiIp",
            "previousWifiMask",
            "previousWifiGw",
            "previousAccessPointPassword",
            "previousApiUser",
            "previousApiPassword",
            "previousDhcp",
        ):
            self.assertIn(snapshot, update)
        network_change = re.search(
            r"const\s+bool\s+networkChanged\s*=\s*(.*?);",
            update,
            flags=re.DOTALL,
        )
        self.assertIsNotNone(network_change)
        expression = network_change.group(1)
        for live_value in (
            "nodeId",
            "wifiSSID",
            "wifiSecret",
            "wifiIp",
            "wifiMask",
            "wifiGw",
            "accessPointPassword",
            "dhcp",
        ):
            self.assertIn(live_value, expression)
        self.assertIn("apiCredentialsChanged", update)
        branch = block_after(
            update,
            r"if\s*\(\s*networkChanged\s*\|\|\s*apiCredentialsChanged\s*\)",
        )
        self.assertIn("restartRequired = true;", branch)

        route = self.server[self.server.index('"/config"') :]
        self.assertIn('root["restartRequired"]', route)
        self.assertIn('response->addHeader("Connection", "close")', route)
        self.assertIn("request->onDisconnect", route)
        self.assertIn("config.requestRestart();", route)

        api = block_after(self.panel, r"async\s+function\s+api\s*\(")
        self.assertIn("Date.now() + 2000", api)
        self.assertIn("res.status !== 409", api)
        self.assertIn("Date.now() >= busyDeadline", api)
        self.assertIn("setTimeout(resolve, 120)", api)

    def test_dhcp_metadata_save_does_not_turn_the_live_lease_into_static_config(self) -> None:
        save = block_after(self.panel, r"async\s+function\s+save\s*\(")
        self.assertOrdered(
            save,
            'body.dhcp = $("s-dhcp").checked;',
            "if (body.dhcp)",
            "delete body.wifiIp;",
            "delete body.wifiMask;",
            "delete body.wifiGw;",
        )

        update = block_after(
            self.config, r"ConfigUpdateResult\s+ConfigOnofre::update\s*\("
        )
        for field, value_name in (
            ("wifiIp", "wifiIpValue"),
            ("wifiMask", "wifiMaskValue"),
            ("wifiGw", "wifiGwValue"),
        ):
            self.assertIn(f'JsonVariantConst {value_name} = root["{field}"];', update)
            self.assertRegex(
                update,
                rf"if\s*\(\s*!dhcp\s*&&\s*!{value_name}\.isUnbound\(\)\s*\)",
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
