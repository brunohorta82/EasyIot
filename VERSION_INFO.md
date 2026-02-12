# EasyIot Version Info

**Created by**: Alexandru Hauzman  
**Created Date**: 11.02.2026

## Summary

This file tracks EasyIot version progress and release notes.
Current development line is **9.17-dev**, focused on safer local configuration,
cleaner build flags, and consistent firmware version reporting.

## Modify

### Version 9.17-dev
- Build: load local `platformio_override.ini` via `extra_configs`. - 11.02.2026
- Build: support `${extra.wifi_flags}` so local Wi-Fi defines stay outside git. - 11.02.2026
- Build: use string version define (`VERSION='"${extra.version}"'`) to support `-dev` suffixes. - 11.02.2026
- Firmware: use `String(VERSION)` for API, mDNS, and Home Assistant firmware metadata. - 11.02.2026
- Build script: improve `tools/extra_script.py` parsing for quoted `VERSION` values. - 11.02.2026
- Git: keep `platformio_override.ini` ignored as a local-only file. - 11.02.2026
- Security: stop logging credential values in debug output (`src/CoreWiFi.cpp`, `src/ConfigOnofre.cpp`). - 11.02.2026
- CloudIO: normalize firmware value in cloud config payload (strip `-dev` suffix for backend compatibility). - 12.02.2026
- Webpanel: replace `parseFloat` firmware compare with robust version parsing for `-dev` formats. - 12.02.2026
- Webpanel: remove hardcoded API base URL and use same-origin requests. - 12.02.2026

### Version 9.163 - Stable Release
- Baseline before `9.17-dev` maintenance and build-flow updates. - 

### Planned Next Improvements
- Add `CHANGELOG.md` for release-by-release history.
- Add CI build checks for primary PlatformIO environments.
- Add version/OTA metadata validation checks.
- Document upstream cherry-pick PR workflow in a short guide.

#
