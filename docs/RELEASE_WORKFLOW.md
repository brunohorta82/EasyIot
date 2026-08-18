# EasyIot Release Workflow

This document defines the standard flow for development, upstream cherry-pick PRs, and release validation.

## Branch Naming

- `development`: main working branch in fork.
- `cp-<topic>`: upstream cherry-pick PR branch (example: `cp-warning-cleanup-config-sensors`).
- Keep topic names short and descriptive with kebab-case.

## Upstream CP/PR Flow

1. Work and test on `development`.
2. Commit focused changes.
3. Push `development` to `origin`.
4. Create CP branch from upstream base:
   - `git fetch upstream`
   - `git switch -c cp-<topic> upstream/master`
5. Cherry-pick the commit(s):
   - `git cherry-pick -x <commit_sha>`
6. Resolve conflicts if needed, then continue:
   - `git add <resolved-files>`
   - `GIT_EDITOR=true git cherry-pick --continue`
7. Push CP branch:
   - `git push -u origin cp-<topic>`
8. Open PR from `Hauzman:cp-<topic>` to `brunohorta82:master`.

## Release Checklist

1. Confirm version string in `platformio.ini` (`[extra] version = ...`).
2. Ensure local secrets stay in `platformio_override.ini` only.
3. Validate release metadata:
   - `tools/validate_release.sh`
   - strict release mode: `tools/validate_release.sh --release --fail-on-http`
4. Build release targets:
   - `platformio run -e ESP8266_RELEASE`
   - `platformio run -e ESP32_RELEASE`
5. Validate key runtime checks on device:
   - Wi-Fi connect
   - CloudIO HTTP result
   - MQTT connect
   - basic actuator/sensor control
6. Update `CHANGELOG.md` for the release/dev line.
7. Push `development` and prepare required CP PR(s).
8. Keep open CP PRs minimal and grouped by scope.

## Project Checks

Run the fast, non-destructive source checks before committing:

- `python3 tools/check_project.py --quick`

Run the same checks and one selected PlatformIO environment:

- `python3 tools/check_project.py --build ESP8266_DEBUG`

Run the recommended build matrix for the current version:

- `python3 tools/check_project.py --all`

For a version containing `-dev`, `--all` builds `ESP8266_DEBUG`,
`ESP8266-HAN_DEBUG`, and `ESP32_DEBUG`. For a release version, it builds the
three environments published by CI: `ESP8266_RELEASE`,
`ESP8266-HAN_RELEASE`, and `ESP32_RELEASE`.

The quick checks validate Python and JavaScript syntax, host tests, release
metadata, generated web-header parity, conflict markers, and whitespace. Web
asset parity is checked in a temporary directory, so the command does not
rewrite working-tree headers.

## Local Wi-Fi Override

1. Copy `platformio_override.example.ini` to `platformio_override.ini`.
2. Uncomment `WIFI_SSID` and `WIFI_SECRET` in the local copy and replace the placeholders.
3. Never commit `platformio_override.ini`; it is intentionally ignored by Git.
4. Keep `platformio.ini` free of real Wi-Fi credentials.

## Practical Notes

- Prefer one CP per logical scope (security, webpanel, docs, etc.).
- If a PR is closed or merged, delete local/remote CP branch to keep repo clean.
- If `git cherry-pick --continue` fails due to editor, use:
  - `GIT_EDITOR=true git cherry-pick --continue`
- Build hooks are automatic via `tools/extra_script.py`:
  - HTML converter runs before build
  - Release metadata validation runs before build
- Optional skip defines in `platformio.ini` (`[extra] default_flags`):
  - `SKIP_HTML_CONVERT`
  - `SKIP_RELEASE_VALIDATE`

## Release Notes Draft

- Generate a draft from recent commits:
  - `python tools/generate_release_notes.py` (Windows: `py -3 tools/generate_release_notes.py`)
- Generate from a specific range:
  - `python tools/generate_release_notes.py --range <from..to>`
- Change output file:
  - `python tools/generate_release_notes.py --count 25 --output RELEASE_NOTES_DRAFT.md`
- Default output path is repo root:
  - `RELEASE_NOTES_DRAFT.md`
- Shell wrapper (macOS/Linux):
  - `tools/generate_release_notes.sh`
