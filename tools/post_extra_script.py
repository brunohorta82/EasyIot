"""PlatformIO post-build hook for local firmware candidate exports."""

import os
import subprocess
import sys

from SCons.Script import Action, DefaultEnvironment  # type: ignore


env = DefaultEnvironment()

ESP8266_BUDGET_ENVIRONMENTS = {
    "ESP8266_RELEASE",
    "ESP8266-HAN_RELEASE",
    "ESP8266_DEBUG",
    "ESP8266-HAN_DEBUG",
}


def check_esp8266_ram_budget(env):
    pioenv = env.get("PIOENV", "")
    if pioenv not in ESP8266_BUDGET_ENVIRONMENTS:
        return 0

    project_dir = env["PROJECT_DIR"]
    script_path = os.path.join(project_dir, "tools", "check_ram_budget.py")
    elf_path = env.subst("$PROGPATH")
    size_tool = env.subst("$SIZETOOL")
    if not os.path.isfile(script_path):
        print("ESP8266 RAM budget check failed: check_ram_budget.py not found")
        return 1

    print("")
    print("Checking ESP8266 static-RAM budget ...")
    result = subprocess.run(
        [
            sys.executable,
            script_path,
            "--env",
            pioenv,
            "--elf",
            elf_path,
            "--size-tool",
            size_tool,
        ],
        check=False,
    )
    return result.returncode


def export_firmware_candidate(target, source, env):
    pioenv = env.get("PIOENV", "")
    if check_esp8266_ram_budget(env) != 0:
        print("Firmware candidate not published: ESP8266 static-RAM budget failed.")
        return 1
    project_dir = env["PROJECT_DIR"]
    script_path = os.path.join(project_dir, "tools", "export_firmware.py")
    binary_path = env.subst("$BUILD_DIR/${PROGNAME}.bin")
    if not os.path.isfile(script_path):
        print("Firmware export failed: export_firmware.py not found.")
        return 1
    if not os.path.isfile(binary_path):
        print(f"Firmware export failed: binary not found: {binary_path}")
        return 1

    print("")
    print("Publishing local firmware candidate ...")
    subprocess.run(
        [
            sys.executable,
            script_path,
            "publish",
            "--env",
            pioenv,
            "--bin",
            binary_path,
        ],
        check=True,
    )
    print("")
    return 0


pioenv = env.get("PIOENV", "")
if "DEBUG" in pioenv or "RELEASE" in pioenv:
    firmware_binary = env.File(env.subst("$BUILD_DIR/${PROGNAME}.bin"))
    env.AlwaysBuild(firmware_binary)
    env.AddPostAction(
        firmware_binary,
        Action(export_firmware_candidate, "Post-build: local firmware candidate"),
    )
