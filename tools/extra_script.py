import datetime
import os
import subprocess
from SCons.Script import DefaultEnvironment  # type: ignore

env = DefaultEnvironment()


def _parse_flags():
    try:
        return env.ParseFlags(env.get("BUILD_FLAGS", ""))
    except Exception:
        return {}


def _has_define(name):
    flags = _parse_flags()
    for define in flags.get("CPPDEFINES", []):
        if define == name:
            return True
        if isinstance(define, (list, tuple)) and len(define) == 2 and define[0] == name:
            return True
    return False


def run_html_converter(source, target, env):
    if _has_define("SKIP_HTML_CONVERT") or _has_define("NO_WEB_UI"):
        print("")
        print("Skipping html_converter.sh (SKIP_HTML_CONVERT or NO_WEB_UI defined)")
        print("")
        return 0

    project_dir = env["PROJECT_DIR"]
    script_path = os.path.join(project_dir, "tools", "html_converter.sh")

    print("")
    print("Running html_converter.sh ...")
    print("Script path:", script_path)
    print("")

    if not os.path.exists(script_path):
        print("html_converter.sh not found!")
        return 1

    subprocess.run(["/bin/bash", script_path], check=True)
    return 0


def run_release_validation(source, target, env):
    if _has_define("SKIP_RELEASE_VALIDATE"):
        print("")
        print("Skipping validate_release.sh (SKIP_RELEASE_VALIDATE defined)")
        print("")
        return 0

    project_dir = env["PROJECT_DIR"]
    script_path = os.path.join(project_dir, "tools", "validate_release.sh")

    if not os.path.exists(script_path):
        print("validate_release.sh not found, skipping validation.")
        return 0

    print("")
    print("Running validate_release.sh ...")
    print("Script path:", script_path)
    print("")

    cmd = ["/bin/bash", script_path]
    if "RELEASE" in env.get("PIOENV", ""):
        cmd.extend(["--release"])

    subprocess.run(cmd, check=True)
    return 0


# Run before firmware link/build output is generated.
env.AddPreAction("$PROGPATH", run_html_converter)
env.AddPreAction("$PROGPATH", run_release_validation)

# Firmware naming
my_flags = _parse_flags()
version = "unknown"
for define in my_flags.get("CPPDEFINES", []):
    if isinstance(define, (list, tuple)) and len(define) == 2 and define[0] == "VERSION":
        version = define[1]
        if isinstance(version, str):
            version = version.strip('"')
        break

timestamp = datetime.datetime.now().strftime("%d.%m.%Y")
env.Replace(PROGNAME=f"Firmware_{env['PIOENV']}_{version} - {timestamp}")
