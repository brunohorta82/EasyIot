"""PlatformIO post-build hook for local firmware candidate exports."""

import os
import subprocess
import sys

from SCons.Script import Action, DefaultEnvironment  # type: ignore


env = DefaultEnvironment()


def export_firmware_candidate(target, source, env):
    pioenv = env.get("PIOENV", "")
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
