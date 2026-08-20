#!/usr/bin/env python3
"""Source-contract tests for the non-reentrant feature-access lease.

The firmware has no host-side AsyncWebServer/FreeRTOS harness.  These tests
therefore inspect the production sources and protect the ownership boundaries
that a successful build cannot prove.  They do not execute callbacks, schedule
tasks, exercise GPIO, or prove fairness/latency on hardware.
"""

from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
CONFIG_HEADER = ROOT / "include" / "ConfigOnofre.h"
CONFIG_SOURCE = ROOT / "src" / "ConfigOnofre.cpp"
MAIN_SOURCE = ROOT / "src" / "main.cpp"
WEB_SOURCE = ROOT / "src" / "WebServer.cpp"
MQTT_SOURCE = ROOT / "src" / "Mqtt.cpp"
CLOUD_SOURCE = ROOT / "src" / "CloudIO.cpp"
CORE_WIFI_SOURCE = ROOT / "src" / "CoreWiFi.cpp"
TEMPLATES_SOURCE = ROOT / "src" / "Templates.cpp"
IRRIGATION_SOURCE = ROOT / "src" / "Irrigation.cpp"
HA_SOURCE = ROOT / "src" / "HomeAssistantMqttDiscovery.cpp"


def block_after(source: str, pattern: str) -> str:
    """Return the balanced brace block following *pattern*.

    Comments and quoted strings are ignored so log text and JSON cannot close a
    block early.  The returned string excludes the outer braces.
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


def conditional_branches_around(source: str, marker: str) -> tuple[str, str]:
    """Return ESP32 and non-ESP32 branches surrounding *marker*."""

    marker_at = source.index(marker)
    start = source.rfind("#ifdef ESP32", 0, marker_at)
    end = source.find("#endif", marker_at)
    if start < 0 or end < 0:
        raise AssertionError(f"ESP32 conditional not found around {marker!r}")
    conditional = source[start : end + len("#endif")]
    match = re.search(
        r"#ifdef\s+ESP32\s*(.*?)#else\s*(.*?)#endif",
        conditional,
        flags=re.DOTALL,
    )
    if match is None:
        raise AssertionError(f"ESP32/non-ESP32 branches missing around {marker!r}")
    return match.group(1), match.group(2)


def section(source: str, start: str, end: str) -> str:
    """Return the source between two stable route comments."""

    start_at = source.index(start)
    end_at = source.index(end, start_at)
    return source[start_at:end_at]


class FeatureAccessSourceContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = CONFIG_HEADER.read_text(encoding="utf-8")
        cls.config = CONFIG_SOURCE.read_text(encoding="utf-8")
        cls.main = MAIN_SOURCE.read_text(encoding="utf-8")
        cls.web = WEB_SOURCE.read_text(encoding="utf-8")
        cls.mqtt = MQTT_SOURCE.read_text(encoding="utf-8")
        cls.cloud = CLOUD_SOURCE.read_text(encoding="utf-8")
        cls.core_wifi = CORE_WIFI_SOURCE.read_text(encoding="utf-8")
        cls.templates = TEMPLATES_SOURCE.read_text(encoding="utf-8")
        cls.irrigation = IRRIGATION_SOURCE.read_text(encoding="utf-8")
        cls.ha = HA_SOURCE.read_text(encoding="utf-8")

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

    def assertBusyReturns409(self, handler: str) -> None:  # noqa: N802
        """Require a non-blocking BUSY branch with an HTTP 409 response.

        A small response helper is allowed.  If the 409 is not literal in the
        branch, inspect functions called with ``request`` from that branch.
        """

        busy = block_after(
            handler,
            r"if\s*\(\s*!\s*config\.tryBeginFeatureAccess\s*\(\s*\)\s*\)",
        )
        self.assertIn("return", busy, "BUSY handling must leave the callback")
        if "409" in busy:
            return

        helper_names = re.findall(
            r"(?<!->)(?<!\.)\b([A-Za-z_]\w*)\s*\(\s*request\b", busy
        )
        for helper_name in helper_names:
            try:
                helper = block_after(
                    self.web,
                    rf"\b{re.escape(helper_name)}\s*\(\s*AsyncWebServerRequest\s*\*\s*request",
                )
            except AssertionError:
                continue
            if "409" in helper:
                return
        self.fail("BUSY branch does not send HTTP 409 directly or through a helper")

    def assertLeasedWebMutation(self, handler: str) -> None:  # noqa: N802
        self.assertIn("config.tryBeginFeatureAccess()", handler)
        self.assertIn("config.endFeatureAccess();", handler)
        self.assertBusyReturns409(handler)

    def test_single_cross_platform_non_reentrant_lease(self) -> None:
        public_api = self.header[: self.header.index("private:")]
        self.assertIn("bool tryBeginFeatureAccess();", public_api)
        self.assertIn("void endFeatureAccess();", public_api)

        esp32_field, esp8266_field = conditional_branches_around(
            self.header, "featureAccessInProgress"
        )
        self.assertRegex(
            esp32_field,
            r"std::atomic<bool>\s+featureAccessInProgress\s*\{\s*false\s*\}\s*;",
        )
        self.assertRegex(
            esp8266_field,
            r"bool\s+featureAccessInProgress\s*(?:=\s*false|\{\s*false\s*\})\s*;",
        )
        self.assertNotIn("std::atomic", esp8266_field)

        begin = block_after(
            self.config, r"bool\s+ConfigOnofre::tryBeginFeatureAccess\s*\(\s*\)"
        )
        begin_esp32, begin_esp8266 = conditional_branches_around(
            begin, "featureAccessInProgress"
        )
        self.assertIn("compare_exchange_strong", begin_esp32)
        self.assertIn("expected", begin_esp32)
        self.assertIn("true", begin_esp32)
        self.assertIn("return false;", begin_esp32)
        self.assertNotRegex(begin_esp32, r"\b(?:while|delay|yield)\s*\(")
        self.assertRegex(
            begin_esp8266,
            r"if\s*\(\s*featureAccessInProgress\s*\)\s*return\s+false\s*;",
        )
        self.assertIn("featureAccessInProgress = true;", begin_esp8266)
        self.assertNotRegex(begin_esp8266, r"\b(?:while|delay|yield)\s*\(")

        end = block_after(
            self.config, r"void\s+ConfigOnofre::endFeatureAccess\s*\(\s*\)"
        )
        end_esp32, end_esp8266 = conditional_branches_around(
            end, "featureAccessInProgress"
        )
        self.assertRegex(
            end_esp32,
            r"featureAccessInProgress\.store\s*\(\s*false\s*,\s*"
            r"std::memory_order_(?:release|seq_cst)\s*\)",
        )
        self.assertIn("featureAccessInProgress = false;", end_esp8266)

        # A second gate would recreate the nested-owner ambiguity this lease is
        # replacing.  Pending-work flags are separate and remain permitted.
        for obsolete in (
            "pauseFeatureRequests",
            "activeFeatureLoops",
            "configUpdateInProgress",
            "beginFeatureLoop",
            "endFeatureLoop",
            "pauseFeatures",
            "resumeFeatures",
        ):
            self.assertNotIn(obsolete, self.header)
            self.assertNotIn(obsolete, self.config)

    def test_config_writer_delegates_to_the_shared_lease(self) -> None:
        begin = block_after(
            self.config, r"bool\s+ConfigOnofre::tryBeginConfigUpdate\s*\(\s*\)"
        )
        end = block_after(
            self.config, r"void\s+ConfigOnofre::endConfigUpdate\s*\(\s*\)"
        )
        self.assertEqual(begin.count("tryBeginFeatureAccess()"), 1)
        self.assertRegex(begin, r"return\s+tryBeginFeatureAccess\s*\(\s*\)\s*;")
        self.assertNotIn("featureAccessInProgress", begin)
        self.assertEqual(end.count("endFeatureAccess();"), 1)
        self.assertNotIn("featureAccessInProgress", end)

    def test_feature_loops_balance_the_lease_on_every_exit(self) -> None:
        for function_name in ("loopActuators", "loopSensors"):
            loop = block_after(
                self.config,
                rf"void\s+ConfigOnofre::{function_name}\s*\(\s*\)",
            )
            busy_pattern = (
                r"if\s*\(\s*!\s*tryBeginFeatureAccess\s*\(\s*\)\s*\)"
            )
            busy = block_after(loop, busy_pattern)
            self.assertIn("return;", busy)
            busy_at = loop.index(busy)
            after_busy = loop[busy_at + len(busy) :]

            previous_return = -1
            for early_return in re.finditer(r"\breturn\s*;", after_busy):
                release = after_busy.rfind(
                    "endFeatureAccess();", previous_return + 1, early_return.start()
                )
                self.assertGreaterEqual(
                    release,
                    0,
                    f"{function_name} returns after acquisition without releasing",
                )
                previous_return = early_return.end()

            self.assertTrue(
                loop.rstrip().endswith("endFeatureAccess();"),
                f"{function_name} must release after its normal path",
            )
            self.assertEqual(loop.count("tryBeginFeatureAccess()"), 1)
            self.assertGreaterEqual(
                loop.count("endFeatureAccess();"),
                len(list(re.finditer(r"\breturn\s*;", after_busy))) + 1,
            )

    def test_async_web_mutation_roots_lease_and_report_busy(self) -> None:
        captive = block_after(
            self.web,
            r"void\s+handleRequest\s*\(\s*AsyncWebServerRequest\s*\*\s*request\s*\)",
        )
        self.assertLeasedWebMutation(captive)

        handlers = {
            "GET /config": section(self.web, "/*GET CONFIG*/", "/*SAVE CONFIG*/"),
            "POST /features": section(
                self.web, "/*CREATE NEW FEATURE*/", "/*CONTROL ACTUATOR*/"
            ),
            "POST /actuators/control": section(
                self.web,
                "/*CONTROL ACTUATOR*/",
                "/*RESET A METER'S ACCUMULATED ENERGY*/",
            ),
            "POST /sensors/reset-energy": section(
                self.web,
                "/*RESET A METER'S ACCUMULATED ENERGY*/",
                "/*IRRIGATION SCHEDULE*/",
            ),
            "POST /irrigation": section(
                self.web, "/*IRRIGATION SCHEDULE*/", "/*FORCE A PROGRAM NOW*/"
            ),
            "POST /irrigation-run": section(
                self.web, "/*FORCE A PROGRAM NOW*/", "auto irrigationStopHandler"
            ),
            "POST/GET /irrigation-stop": block_after(
                self.web,
                r"auto\s+irrigationStopHandler\s*=\s*\[\]\s*\(\s*AsyncWebServerRequest\s*\*\s*request",
            ),
        }
        for label, handler in handlers.items():
            with self.subTest(route=label):
                self.assertLeasedWebMutation(handler)

        # POST /config owns the same lease through ConfigOnofre::update().  A
        # second acquisition in the route would reject itself as BUSY.
        save_config = section(
            self.web, "/*SAVE CONFIG*/", "/*CREATE NEW FEATURE*/"
        )
        self.assertIn("config.update", save_config)
        self.assertNotIn("tryBeginFeatureAccess", save_config)
        self.assertNotIn("endFeatureAccess", save_config)

    def test_web_authentication_uses_a_protected_credential_snapshot(self) -> None:
        authorize = block_after(
            self.web,
            r"bool\s+authorizeRequest\s*\(\s*AsyncWebServerRequest\s*\*\s*request",
        )
        self.assertEqual(authorize.count("config.tryBeginFeatureAccess()"), 1)
        self.assertEqual(authorize.count("config.endFeatureAccess();"), 1)
        self.assertOrdered(
            authorize,
            "config.tryBeginFeatureAccess()",
            "config.apiUser",
            "config.apiPassword",
            "config.endFeatureAccess();",
            "request->authenticate(user, password, REALM)",
        )
        self.assertIn("if (sendFailure)", authorize)
        self.assertNotRegex(
            self.web,
            r"request->authenticate\s*\(\s*config\.apiUser",
            "async handlers must not read mutable credentials directly",
        )

        manual_state = block_after(self.web, r"struct\s+ManualUpdateState")
        self.assertIn("authenticated", manual_state)

    def test_manual_ota_owns_one_lease_for_the_upload_lifetime(self) -> None:
        state = block_after(self.web, r"struct\s+ManualUpdateState")
        self.assertIn("ownsFeatureAccess", state)
        self.assertNotIn("featuresPaused", state)

        update_route = self.web[self.web.index('"/update", HTTP_POST') :]
        upload = block_after(
            update_route,
            r"\[\]\s*\(\s*AsyncWebServerRequest\s*\*\s*request\s*,\s*String\s+filename\s*,\s*"
            r"size_t\s+index\s*,\s*uint8_t\s*\*\s*data\s*,\s*size_t\s+len\s*,\s*bool\s+final\s*\)",
        )
        first_chunk = block_after(upload, r"if\s*\(\s*!\s*index\s*\)")
        self.assertEqual(upload.count("config.tryBeginFeatureAccess()"), 1)
        self.assertIn("authorizeRequest(request, false, &authBusy)", first_chunk)
        self.assertNotIn("requestAuthentication", first_chunk)
        self.assertIn("config.tryBeginFeatureAccess()", first_chunk)
        self.assertIn("ownsFeatureAccess", first_chunk)
        self.assertRegex(update_route, r"\bbusy\b")
        self.assertIn("409", update_route)
        self.assertIn("config.endFeatureAccess();", upload)
        self.assertNotIn("pauseFeatures", update_route)
        self.assertNotIn("resumeFeatures", update_route)

    def test_main_template_defaults_and_auto_ota_use_one_top_level_lease(self) -> None:
        routines = block_after(self.main, r"void\s+checkInternalRoutines\s*\(\s*\)")

        template = block_after(
            routines,
            r"if\s*\(\s*requestedTemplateId\s*!=\s*Template::NO_TEMPLATE\s*\)",
        )
        self.assertEqual(template.count("config.tryBeginFeatureAccess()"), 1)
        self.assertNotIn("config.requestTemplateChange(requestedTemplateId);", template)
        self.assertIn("config.clearTemplateChangeRequest(requestedTemplateId);", template)
        self.assertIn("config.endFeatureAccess();", template)

        defaults = block_after(
            routines,
            r"if\s*\(\s*config\.isLoadDefaultsRequested\s*\(\s*\)\s*\)",
        )
        self.assertEqual(defaults.count("config.tryBeginFeatureAccess()"), 1)
        self.assertIn("config.requestLoadDefaults();", defaults)
        # Successful reset retains ownership until the requested restart.
        self.assertNotIn("config.endFeatureAccess();", defaults)

        auto_ota = block_after(
            routines,
            r"if\s*\(\s*config\.takeAutoUpdateRequest\s*\(\s*\)\s*\)",
        )
        self.assertEqual(auto_ota.count("config.tryBeginFeatureAccess()"), 1)
        self.assertIn("config.requestAutoUpdate();", auto_ota)
        self.assertIn("config.endFeatureAccess();", auto_ota)
        self.assertOrdered(
            auto_ota,
            "performUpdate();",
            "if (updateResult == AutoUpdateResult::UPDATED)",
            "config.requestRestart();",
            "config.endFeatureAccess();",
            "startWebserver();",
        )

        self.assertNotIn("pauseFeatures", routines)
        self.assertNotIn("resumeFeatures", routines)

    def test_local_mqtt_loop_is_the_single_synchronous_root(self) -> None:
        local_callback = block_after(
            self.mqtt,
            r"void\s+callbackMqtt\s*\(\s*char\s*\*\s*topic",
        )
        reconnect = block_after(self.mqtt, r"boolean\s+reconnect\s*\(\s*\)")

        # PubSubClient invokes callbackMqtt synchronously from loop(). Both the
        # callback and reconnect are therefore leaves beneath one loop owner.
        for label, leaf in (
            ("local MQTT callback", local_callback),
            ("local MQTT reconnect", reconnect),
        ):
            with self.subTest(leaf=label):
                self.assertNotIn("tryBeginFeatureAccess", leaf)
                self.assertNotIn("endFeatureAccess", leaf)
        self.assertIn("config.controlFeature", local_callback)
        self.assertIn("initHomeAssistantDiscovery", local_callback)
        self.assertIn("for (auto &sw : config.actuatores)", reconnect)

        mqtt_loop = block_after(self.mqtt, r"void\s+loopMqtt\s*\(\s*\)")
        self.assertEqual(mqtt_loop.count("config.tryBeginFeatureAccess()"), 1)
        self.assertEqual(mqtt_loop.count("config.endFeatureAccess();"), 2)
        busy = block_after(
            mqtt_loop,
            r"if\s*\(\s*!\s*config\.tryBeginFeatureAccess\s*\(\s*\)\s*\)",
        )
        self.assertIn("return", busy)
        unavailable = block_after(
            mqtt_loop,
            r"if\s*\(\s*!\s*wifiConnected\s*\(\s*\)\s*\|\|\s*strlen\s*\(\s*config\.mqttIpDns\s*\)\s*==\s*0\s*\)",
        )
        self.assertOrdered(unavailable, "config.endFeatureAccess();", "return;")
        self.assertIn("reconnect()", mqtt_loop)
        self.assertIn("mqttClient.loop();", mqtt_loop)
        self.assertTrue(mqtt_loop.rstrip().endswith("config.endFeatureAccess();"))

    def test_cloud_callbacks_only_signal_or_enqueue(self) -> None:
        cloud_connect = block_after(
            self.cloud, r"void\s+onMqttConnect\s*\(\s*bool\s+sessionPresent\s*\)"
        )
        cloud_disconnect = block_after(
            self.cloud,
            r"void\s+onMqttDisconnect\s*\(\s*AsyncMqttClientDisconnectReason",
        )
        cloud_message = block_after(
            self.cloud, r"void\s+onMqttMessage\s*\(\s*char\s*\*\s*topic"
        )

        for label, callback in (
            ("connect", cloud_connect),
            ("disconnect", cloud_disconnect),
            ("message", cloud_message),
        ):
            with self.subTest(callback=label):
                self.assertNotIn("tryBeginFeatureAccess", callback)
                self.assertNotIn("endFeatureAccess", callback)
                self.assertNotIn("Log.", callback)
                self.assertNotRegex(
                    callback,
                    r"mqttClient\.(?:connect|disconnect|publish|subscribe)\s*\(",
                )

        self.assertIn("signalCloudMqttTransportEvent", cloud_connect)
        self.assertIn("CloudMqttTransportEvent::CONNECTED", cloud_connect)
        self.assertNotIn("storeCloudMqttConnectedState", cloud_connect)
        self.assertIn("signalCloudMqttTransportEvent", cloud_disconnect)
        self.assertIn("CloudMqttTransportEvent::DISCONNECTED", cloud_disconnect)
        self.assertIn("index != 0", cloud_message)
        self.assertIn("len != total", cloud_message)
        self.assertIn("len >= kCloudCommandPayloadSize", cloud_message)
        self.assertIn("queueCloudIOCommand(topic, payload, len)", cloud_message)
        for forbidden in (
            "config.controlFeature",
            "config.requestRestart",
            "config.requestAutoUpdate",
        ):
            self.assertNotIn(forbidden, cloud_message)

        enqueue = block_after(
            self.cloud,
            r"QueueCloudCommandResult\s+queueCloudIOCommand\s*\(",
        )
        self.assertNotRegex(enqueue, r"\b(?:malloc|calloc|realloc|new)\b")
        self.assertIn("memchr(payload, '\\0', payloadLength)", enqueue)
        self.assertIn("writeIndex - readIndex >= kCloudCommandQueueCapacity", enqueue)
        self.assertIn("publishPendingCloudCommand(writeIndex + 1)", enqueue)
        self.assertRegex(
            self.cloud,
            r"PendingCloudCommand\s+pendingCloudCommands\s*\[\s*kCloudCommandQueueCapacity\s*\]",
        )
        esp32_queue, esp8266_queue = conditional_branches_around(
            self.cloud, "pendingCloudCommandWrite"
        )
        self.assertRegex(
            esp32_queue,
            r"std::atomic<uint32_t>\s+pendingCloudCommandWrite",
        )
        self.assertRegex(
            esp32_queue,
            r"std::atomic<uint32_t>\s+pendingCloudCommandRead",
        )
        self.assertNotIn("std::atomic", esp8266_queue)
        self.assertRegex(esp8266_queue, r"uint32_t\s+pendingCloudCommandWrite")
        self.assertRegex(esp8266_queue, r"uint32_t\s+pendingCloudCommandRead")

    def test_cloud_main_services_are_ordered_and_command_drain_is_bounded(self) -> None:
        routines = block_after(self.main, r"void\s+checkInternalRoutines\s*\(\s*\)")
        self.assertOrdered(
            routines,
            "serviceCloudIOMqtt();",
            "serviceCloudIOWatchdog();",
            "drainCloudIOCommands();",
            "config.peekTemplateChangeRequest();",
        )

        mqtt_service = block_after(
            self.cloud, r"void\s+serviceCloudIOMqtt\s*\(\s*\)"
        )
        self.assertIn("takeCloudMqttTransportEvent()", mqtt_service)
        self.assertIn("serviceCloudMqttSubscriptions();", mqtt_service)
        self.assertIn("mqttClient.connect();", mqtt_service)
        self.assertIn("cloudMqttTransitionTimedOut()", mqtt_service)
        self.assertIn("requestCloudMqttRecoveryRestart();", mqtt_service)
        connected_branch = block_after(
            mqtt_service,
            r"if\s*\(\s*event\s*==\s*CloudMqttTransportEvent::CONNECTED\s*\)",
        )
        self.assertOrdered(
            connected_branch,
            "storeCloudMqttConnectedState(true);",
            "cloudMqttSubscriptionsPending = true;",
        )
        self.assertOrdered(
            mqtt_service,
            "config.tryBeginFeatureAccess()",
            "storeCloudMqttConnectedState(false);",
            "config.endFeatureAccess();",
            "mqttClient.disconnect(true);",
        )

        transition = block_after(
            self.cloud,
            r"void\s+enterCloudMqttTransportState\s*\(",
        )
        self.assertIn("CloudMqttTransportState::CONNECTING", transition)
        self.assertIn("CloudMqttTransportState::DISCONNECTING", transition)
        self.assertIn("cloudMqttTransitionStartedAt = millis();", transition)
        timeout = block_after(
            self.cloud,
            r"bool\s+cloudMqttTransitionTimedOut\s*\(\s*\)",
        )
        self.assertIn("millis() - cloudMqttTransitionStartedAt", timeout)
        self.assertIn("kCloudMqttTransitionTimeoutMs", timeout)
        self.assertRegex(
            self.cloud,
            r"kCloudMqttTransitionTimeoutMs\s*=\s*30000\s*;",
        )
        recovery = block_after(
            self.cloud,
            r"void\s+requestCloudMqttRecoveryRestart\s*\(\s*\)",
        )
        self.assertIn("cloudMqttRecoveryQueued = true;", recovery)
        self.assertIn("storeCloudMqttConnectedState(false);", recovery)
        self.assertIn("config.requestRestart();", recovery)
        self.assertNotRegex(
            recovery,
            r"mqttClient\.(?:connect|disconnect)\s*\(",
        )

        staged_start = mqtt_service.rfind("if (stagedCloudMqttRuntimePending)")
        self.assertGreaterEqual(staged_start, 0)
        staged = block_after(
            mqtt_service[staged_start:],
            r"if\s*\(\s*stagedCloudMqttRuntimePending\s*\)",
        )
        self.assertOrdered(
            staged,
            "CloudMqttTransportState::CONNECTING",
            "return;",
            "CloudMqttTransportState::CONNECTED",
            "mqttClient.disconnect(true);",
        )

        subscriptions = block_after(
            self.cloud, r"void\s+serviceCloudMqttSubscriptions\s*\(\s*\)"
        )
        self.assertEqual(subscriptions.count("config.tryBeginFeatureAccess()"), 1)
        self.assertEqual(subscriptions.count("config.endFeatureAccess();"), 1)

        drain = block_after(
            self.cloud, r"void\s+drainCloudIOCommands\s*\(\s*\)"
        )
        self.assertEqual(drain.count("config.tryBeginFeatureAccess()"), 1)
        self.assertEqual(drain.count("config.endFeatureAccess();"), 1)
        self.assertNotRegex(drain, r"\b(?:for|while)\s*\(")
        self.assertIn("Queue", self.cloud)
        self.assertIn("overflowedCount", drain)
        self.assertIn('strcmp(command.payload, "REBOOT")', drain)
        self.assertIn('strcmp(command.payload, "UPDATE")', drain)
        self.assertIn("config.requestRestart();", drain)
        self.assertIn("config.requestAutoUpdate();", drain)
        self.assertIn("config.controlFeature", drain)
        self.assertOrdered(
            drain,
            "config.tryBeginFeatureAccess()",
            "PendingCloudCommand &command",
            "releasePendingCloudCommand(readIndex + 1);",
            "config.endFeatureAccess();",
        )

    def test_template_request_slot_cannot_overwrite_pending_work(self) -> None:
        request = block_after(
            self.config, r"bool\s+ConfigOnofre::requestTemplateChange\s*\("
        )
        self.assertIn("int expected = Template::NO_TEMPLATE;", request)
        self.assertIn("compare_exchange_strong", request)
        self.assertIn("requestedTemplateId != Template::NO_TEMPLATE", request)

        peek = block_after(
            self.config, r"int\s+ConfigOnofre::peekTemplateChangeRequest\s*\("
        )
        self.assertIn("requestedTemplateId.load(std::memory_order_acquire)", peek)
        self.assertNotIn("exchange(", peek)

        routines = block_after(self.main, r"void\s+checkInternalRoutines\s*\(\s*\)")
        self.assertIn("config.peekTemplateChangeRequest();", routines)
        self.assertNotIn("config.requestTemplateChange(requestedTemplateId);", routines)

        route = block_after(self.web, r"auto\s+templateChangeHandler\s*=\s*\[\]")
        self.assertIn("if (!config.requestTemplateChange(templateId))", route)
        busy = block_after(
            route, r"if\s*\(\s*!\s*config\.requestTemplateChange\(templateId\)\s*\)"
        )
        self.assertIn("sendFeatureBusy(request);", busy)

        watchdog_callback = block_after(
            self.cloud, r"void\s+watchdogTimer\s*\(\s*\)"
        )
        self.assertIn("signalCloudIOWatchdog();", watchdog_callback)
        for forbidden in ("Log.", "wifiConnected", "config.", "mqttClient."):
            self.assertNotIn(forbidden, watchdog_callback)

        watchdog_service = block_after(
            self.cloud, r"void\s+serviceCloudIOWatchdog\s*\(\s*\)"
        )
        self.assertIn("takeCloudIOWatchdogSignal()", watchdog_service)
        self.assertIn("wifiConnected()", watchdog_service)
        self.assertIn("cloudIOConnected()", watchdog_service)
        self.assertIn("config.requestCloudIOSync();", watchdog_service)

    def test_cloud_sync_uses_short_snapshot_and_apply_leases(self) -> None:
        sync = block_after(self.cloud, r"void\s+connectToCloudIO\s*\(\s*\)")
        self.assertGreaterEqual(sync.count("config.tryBeginFeatureAccess()"), 2)
        self.assertGreaterEqual(sync.count("config.endFeatureAccess();"), 2)
        self.assertGreaterEqual(sync.count("config.requestCloudIOSync();"), 2)

        # Never hold the lease through TLS retries and HTTP backoff.
        self.assertOrdered(
            sync,
            "config.tryBeginFeatureAccess()",
            "config.json(root, false);",
            "config.endFeatureAccess();",
            "postCloudConfig",
        )

        ok_branch = block_after(
            sync, r"else\s+if\s*\(\s*httpCode\s*==\s*HTTP_CODE_OK\s*\)"
        )
        self.assertOrdered(
            ok_branch,
            "config.tryBeginFeatureAccess()",
            "config.cloudIOUsername",
            "for (auto &sw : config.actuatores)",
            "for (auto &ss : config.sensors)",
            "stageCloudMqttRuntimeLocked();",
            "config.endFeatureAccess();",
        )

        stage = block_after(
            self.cloud, r"void\s+stageCloudMqttRuntimeLocked\s*\(\s*\)"
        )
        self.assertIn("runtime.clientId", stage)
        self.assertIn("config.chipId", stage)
        self.assertIn("runtime.username", stage)
        self.assertIn("config.cloudIOUsername", stage)
        self.assertIn("runtime.password", stage)
        self.assertIn("config.cloudIOPassword", stage)
        self.assertIn("runtime.healthTopic", stage)
        self.assertIn("config.cloudIOhealthTopic", stage)

        promote = block_after(
            self.cloud, r"void\s+promoteStagedCloudMqttRuntime\s*\(\s*\)"
        )
        self.assertIn("mqttClient.setWill(runtime.healthTopic", promote)
        self.assertIn("mqttClient.setClientId(runtime.clientId)", promote)
        self.assertIn(
            "mqttClient.setCredentials(runtime.username, runtime.password)", promote
        )
        for mutable_config_pointer in (
            "config.chipId",
            "config.cloudIOUsername",
            "config.cloudIOPassword",
            "config.cloudIOhealthTopic",
        ):
            self.assertNotIn(mutable_config_pointer, promote)

    def test_core_wifi_stages_provisioning_credentials_and_retries_busy_work(self) -> None:
        queue = block_after(
            self.core_wifi, r"void\s+queuePendingWiFiCredentials\s*\("
        )
        self.assertOrdered(
            queue,
            "strlcpy(ssidCopy",
            "strlcpy(secretCopy",
            "lockPendingWiFi();",
            "memcpy(pendingWiFiCredentials.ssid",
            "memcpy(pendingWiFiCredentials.secret",
            "pendingWiFiCredentials.credentialsPending = true;",
            "unlockPendingWiFi();",
        )

        drain = block_after(
            self.core_wifi,
            r"static\s+void\s+drainPendingProvisioningCredentials\s*\(\s*\)",
        )
        self.assertOrdered(
            drain,
            "hasPendingWiFiWork()",
            "config.tryBeginFeatureAccess()",
            "takePendingWiFiWork()",
            "config.wifiSSID",
            "config.wifiSecret",
            "if (pending.savePending && !config.persist())",
            "queuePendingWiFiSave();",
            "config.endFeatureAccess();",
        )
        self.assertNotIn("config.save();", drain)

        provision_callback = block_after(
            self.core_wifi,
            r"void\s+SysProvEvent\s*\(\s*arduino_event_t\s*\*\s*sys_event",
        )
        self.assertIn("queuePendingWiFiCredentials", provision_callback)
        self.assertIn("queuePendingWiFiSave", provision_callback)
        self.assertNotIn("strlcpy(config.wifiSSID", provision_callback)
        self.assertNotIn("strlcpy(config.wifiSecret", provision_callback)
        self.assertNotIn("config.save();", provision_callback)
        self.assertNotIn("drainPendingProvisioningCredentials();", provision_callback)

        wifi_loop = block_after(self.core_wifi, r"void\s+loopWiFi\s*\(\s*\)")
        self.assertIn("drainPendingProvisioningCredentials();", wifi_loop)

    def test_core_wifi_defers_mdns_and_uses_protected_snapshots(self) -> None:
        snapshot = block_after(
            self.core_wifi, r"bool\s+takeWiFiConfigSnapshot\s*\("
        )
        self.assertEqual(snapshot.count("config.tryBeginFeatureAccess()"), 1)
        self.assertEqual(snapshot.count("config.endFeatureAccess();"), 1)
        self.assertOrdered(
            snapshot,
            "config.tryBeginFeatureAccess()",
            "config.nodeId",
            "config.chipId",
            "config.wifiSSID",
            "config.wifiSecret",
            "config.endFeatureAccess();",
        )

        mdns_callback = block_after(
            self.core_wifi,
            r"void\s+mdnsCallback\s*\(\s*justwifi_messages_t",
        )
        self.assertIn("requestMDNSRefresh();", mdns_callback)
        self.assertNotIn("refreshMDNS();", mdns_callback)

        refresh = block_after(self.core_wifi, r"void\s+refreshMDNS\s*\(\s*\)")
        self.assertOrdered(
            refresh,
            "takeWiFiConfigSnapshot(snapshot)",
            "requestMDNSRefresh();",
            "MDNS.begin",
        )
        for direct_config_read in (
            "config.nodeId",
            "config.chipId",
            "config.wifiSSID",
        ):
            self.assertNotIn(direct_config_read, refresh)

        reload_wifi = block_after(
            self.core_wifi, r"void\s+reloadWiFiConfig\s*\(\s*\)"
        )
        self.assertOrdered(
            reload_wifi,
            "takeWiFiConfigSnapshot(snapshot)",
            "config.requestReloadWifi();",
            "jw.disconnect();",
        )
        self.assertNotIn("config.wifiSSID", reload_wifi)
        self.assertNotIn("config.wifiSecret", reload_wifi)

        wifi_loop = block_after(self.core_wifi, r"void\s+loopWiFi\s*\(\s*\)")
        self.assertOrdered(
            wifi_loop,
            "takeMDNSRefreshRequest()",
            "refreshMDNS();",
        )

    def test_leaf_helpers_never_acquire_the_non_reentrant_lease(self) -> None:
        leaves: list[tuple[str, str]] = [
            (
                "ConfigOnofre::loadTemplate",
                block_after(
                    self.config,
                    r"bool\s+ConfigOnofre::loadTemplate\s*\(\s*int\s+templateId\s*\)",
                ),
            ),
            (
                "ConfigOnofre::save",
                block_after(self.config, r"ConfigOnofre\s*&ConfigOnofre::save\s*\(\s*\)"),
            ),
            (
                "ConfigOnofre::reloadFeatures",
                block_after(
                    self.config,
                    r"ConfigOnofre\s*&ConfigOnofre::reloadFeatures\s*\(\s*\)",
                ),
            ),
            (
                "ConfigOnofre::json",
                block_after(
                    self.config,
                    r"void\s+ConfigOnofre::json\s*\(\s*JsonVariant\s*&\s*root",
                ),
            ),
            (
                "ConfigOnofre::controlFeature(Json)",
                block_after(
                    self.config,
                    r"void\s+ConfigOnofre::controlFeature\s*\(\s*StateOrigin\s+origin\s*,\s*JsonObject",
                ),
            ),
            (
                "ConfigOnofre::controlFeature(topic)",
                block_after(
                    self.config,
                    r"void\s+ConfigOnofre::controlFeature\s*\(\s*StateOrigin\s+origin\s*,\s*String\s+topic",
                ),
            ),
            ("performUpdate", block_after(self.web, r"AutoUpdateResult\s+performUpdate\s*\(\s*\)")),
            (
                "Irrigation::update",
                block_after(
                    self.irrigation,
                    r"bool\s+Irrigation::update\s*\(\s*JsonObject\s*&\s*root\s*\)",
                ),
            ),
            ("Irrigation::stop", block_after(self.irrigation, r"void\s+Irrigation::stop\s*\(\s*\)")),
            (
                "Irrigation::runProgram",
                block_after(
                    self.irrigation,
                    r"bool\s+Irrigation::runProgram\s*\(\s*uint8_t\s+programId\s*\)",
                ),
            ),
            (
                "initHomeAssistantDiscovery",
                block_after(self.ha, r"void\s+initHomeAssistantDiscovery\s*\(\s*\)"),
            ),
        ]

        for match in re.finditer(r"^(?:int|void|bool)\s+(prepare\w+)\s*\(", self.templates, re.MULTILINE):
            leaves.append(
                (
                    match.group(1),
                    block_after(
                        self.templates,
                        rf"^(?:int|void|bool)\s+{re.escape(match.group(1))}\s*\(",
                    ),
                )
            )
        leaves.append(
            ("templateSelect", block_after(self.templates, r"bool\s+templateSelect\s*\("))
        )

        for label, leaf in leaves:
            with self.subTest(leaf=label):
                self.assertNotIn("tryBeginFeatureAccess", leaf)
                self.assertNotIn("endFeatureAccess", leaf)


if __name__ == "__main__":
    unittest.main(verbosity=2)
