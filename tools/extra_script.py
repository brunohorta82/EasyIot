import datetime
from SCons.Script import Import, DefaultEnvironment # type: ignore

# Import the environment
env = DefaultEnvironment()

try:
    my_flags = env.ParseFlags(env['BUILD_FLAGS'])
except Exception:
    my_flags = {}

# Extract version
version = None
for define in my_flags.get("CPPDEFINES", []):
    if isinstance(define, (list, tuple)) and define[0] == "VERSION":
        version = define[1]
        if isinstance(version, str):
            version = version.strip('"')

if not version:
    version = "unknown"

# Generate firmware name with date in DD.MM.YYYY format
timestamp = datetime.datetime.now().strftime("%d.%m.%Y")
firmware_name = f"Firmware_{env['PIOENV']}_{version} - {timestamp}"

# Replace the program name
env.Replace(PROGNAME=firmware_name)
