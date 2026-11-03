#!/usr/bin/env python3
"""Source contracts for surviving an unwell cloud.

Nothing local needs the cloud, so nothing local may wait for it. These three
properties were each a real outage: a device whose physical switches stopped
answering while a server far away was slow, and a device that restarted every
thirty seconds and so never finished starting at all.
"""
from __future__ import annotations

import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CLOUD = (ROOT / "src" / "CloudIO.cpp").read_text(encoding="utf-8")
MAIN = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
DEVICE_LOG = (ROOT / "src" / "DeviceLog.cpp").read_text(encoding="utf-8")


def block_after(source: str, pattern: str) -> str:
    """The braced block that follows a match, so a contract is checked against
    the function it belongs to and not the whole file."""
    match = re.search(pattern, source)
    if match is None:
        raise AssertionError(f"pattern not found: {pattern}")
    start = source.index("{", match.end())
    depth = 0
    for i in range(start, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise AssertionError(f"unbalanced block after: {pattern}")


class CloudOutageResilience(unittest.TestCase):

    # --- the sync must not block the main loop -----------------------------

    def test_sync_runs_on_its_own_task_where_there_is_one(self) -> None:
        # The POST can take tens of seconds against a server that accepts the
        # connection and then does not answer. On the loop, the web panel, the
        # local MQTT and the actuators all wait behind it.
        self.assertIn("bool dispatchCloudIOSync()", CLOUD)
        self.assertIn("xTaskCreate(cloudSyncTask", CLOUD)
        dispatch = block_after(CLOUD, r"bool\s+dispatchCloudIOSync\s*\(\s*\)")
        self.assertIn("xTaskNotifyGive", dispatch)

    def test_the_loop_dispatches_rather_than_calling_inline(self) -> None:
        requested = block_after(MAIN, r"if\s*\(\s*config\.isCloudIOSyncRequested\(\)\s*\)")
        self.assertIn("dispatchCloudIOSync()", requested)
        # The fallback matters: a device that cannot create the task must still
        # sync, even if it does so the slow way.
        self.assertIn("connectToCloudIO();", requested)

    def test_the_single_threaded_board_keeps_its_budget_small(self) -> None:
        # An ESP8266 has nowhere to move the call to, so what it costs is
        # capped instead: one attempt, and a short one.
        esp8266 = CLOUD[CLOUD.index("#ifdef ESP8266"):]
        timeout = re.search(r"const uint16_t kCloudIoRequestTimeoutMs = (\d+);", esp8266)
        self.assertIsNotNone(timeout)
        self.assertLessEqual(int(timeout.group(1)), 6000)
        retries = re.search(r"const uint8_t kCloudIoHttpsRetryCount = (\d+);", esp8266)
        self.assertIsNotNone(retries)
        self.assertEqual(int(retries.group(1)), 1)

    # --- a failed sync must back off ---------------------------------------

    def test_failed_syncs_back_off_instead_of_retrying_every_minute(self) -> None:
        self.assertIn("nextCloudSyncBackoffTicks", CLOUD)
        backoff = block_after(CLOUD, r"static uint8_t nextCloudSyncBackoffTicks\s*\(\s*\)")
        # Doubling, and bounded: the watchdog ticks once a minute, so an
        # unbounded wait would be a device that never comes back on its own.
        self.assertIn("<<=", backoff)
        self.assertIn("kCloudSyncMaxBackoffTicks", backoff)

    def test_a_working_sync_clears_the_backoff(self) -> None:
        # Anchored on the pair rather than on a branch: "httpCode ==
        # HTTP_CODE_OK" appears more than once, and the first match is the
        # not-adopted path, which sets a backoff instead of clearing one.
        self.assertRegex(
            CLOUD,
            r"cloudSyncBackoffTicks = 0;\s*\n\s*cloudSyncFailureStreak = 0;",
        )
        # And the streak is only ever cleared on success, never on a failure
        # that happens to pass through. One of the two hits is the declaration.
        assignments = re.findall(r"^\s*cloudSyncFailureStreak = 0;", CLOUD, re.M)
        self.assertEqual(len(assignments), 1)

    # --- the cloud must never cost the device its boot ---------------------

    def test_recovery_restarts_are_capped(self) -> None:
        recovery = block_after(CLOUD, r"void\s+requestCloudMqttRecoveryRestart\s*\(\s*\)")
        self.assertIn("kCloudMqttMaxRecoveryRestarts", recovery)
        # Past the cap it returns WITHOUT restarting: the device runs locally
        # rather than rebooting into the same outage forever.
        give_up = recovery[recovery.index("if (restarts > kCloudMqttMaxRecoveryRestarts)"):]
        self.assertLess(give_up.index("return;"), give_up.index("config.requestRestart();"))

    def test_the_counter_survives_a_restart_but_not_a_power_cut(self) -> None:
        # A counter in RAM resets on the very restart it is counting, so it
        # would never reach the cap. Flash is the other wrong answer: a restart
        # loop would write to it every thirty seconds.
        self.assertIn("RTC_NOINIT_ATTR", CLOUD)
        self.assertNotIn("kCloudRecoveryPath", CLOUD)
        read = block_after(CLOUD, r"static uint32_t readCloudRecoveryRestarts\s*\(\s*\)")
        # RTC cells keep their contents through a power-on, so a magic value is
        # what separates a real count from whatever was already there.
        self.assertIn("kCloudRecoveryMagic", read)

    def test_a_connection_clears_the_restart_streak(self) -> None:
        self.assertIn("void clearCloudMqttRecoveryRestarts()", CLOUD)
        connected = CLOUD[CLOUD.index('deviceLog("cloud mqtt ligado");'):]
        self.assertIn("clearCloudMqttRecoveryRestarts();", connected[:400])

    # --- the log is now written from more than one context -----------------

    def test_the_device_log_is_guarded_where_there_are_tasks(self) -> None:
        # The ring buffer is written from the loop, the sensor task and now the
        # cloud task. Two writers racing on nextLine lose a line and can leave
        # the index past the end.
        self.assertIn("SemaphoreHandle_t logMutex", DEVICE_LOG)
        for fn in (r"void\s+deviceLog\s*\(const char \*format",
                   r"String\s+deviceLogText\s*\(\s*\)",
                   r"void\s+deviceLogClear\s*\(\s*\)"):
            self.assertIn("LogGuard guard;", block_after(DEVICE_LOG, fn))


if __name__ == "__main__":
    unittest.main(verbosity=0, exit=False, argv=[sys.argv[0]])
