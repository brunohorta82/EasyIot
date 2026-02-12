#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

usage() {
  cat <<'EOF'
Validate release/version metadata before publishing firmware.

Usage:
  tools/validate_release.sh [--version <value>] [--env <name>] [--release] [--fail-on-http]

Options:
  --version, -v     Override version value (otherwise read from platformio.ini [extra] version)
  --env, -e         Validate WEB_SECURE policy for a single environment (e.g. ESP8266_RELEASE)
  --release, -r     Strict mode for release builds (fails when version contains "-dev")
  --fail-on-http    Fail if CloudIO config/OTA URLs still use http://
  --help, -h        Show help
EOF
}

version=""
target_env=""
strict_release=0
fail_on_http=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --version|-v)
      version="${2:-}"
      shift 2
      ;;
    --env|-e)
      target_env="${2:-}"
      shift 2
      ;;
    --release|-r)
      strict_release=1
      shift
      ;;
    --fail-on-http)
      fail_on_http=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if [[ -z "$version" ]]; then
  version="$(
    awk '
      $0 ~ /^\[extra\]/ { in_extra=1; next }
      /^\[/ { in_extra=0 }
      in_extra && $1=="version" && $2=="=" { print $3; exit }
    ' platformio.ini
  )"
fi

failures=0
warnings=0

ok() {
  echo "[OK] $1"
}

warn() {
  echo
  echo "[WARN] $1"
  echo
  warnings=$((warnings + 1))
}

fail() {
  echo "[FAIL] $1"
  failures=$((failures + 1))
}

if [[ -z "$version" ]]; then
  fail "Could not read version from platformio.ini [extra] section."
else
  ok "Version detected: ${version}"
fi

if [[ -n "$version" ]]; then
  if [[ "$version" =~ ^[0-9]+(\.[0-9]+){1,2}(-[A-Za-z0-9._-]+)?$ ]]; then
    ok "Version format looks valid."
  else
    fail "Version format is invalid: ${version}"
  fi

  if [[ $strict_release -eq 1 && "$version" == *"-dev"* ]]; then
    fail "Strict release mode requires non-dev version (found: ${version})."
  fi
fi

if [[ -n "$version" ]]; then
  if grep -Fq "## [${version}]" CHANGELOG.md; then
    ok "CHANGELOG.md contains entry for version ${version}."
  else
    fail "CHANGELOG.md is missing section header: ## [${version}]"
  fi
fi

required_envs=(
  "env:ESP8266_RELEASE"
  "env:ESP32_RELEASE"
)

for env_name in "${required_envs[@]}"; do
  if grep -Fq "[${env_name}]" platformio.ini; then
    ok "Found required PlatformIO environment: ${env_name}"
  else
    fail "Missing required PlatformIO environment: ${env_name}"
  fi
done

section_has_unflag_macro() {
  local env_name="$1"
  local macro_name="$2"

  awk -v section="[${env_name}]" -v macro="${macro_name}" '
    $0 == section {
      in_section = 1
      in_unflags = 0
      next
    }

    in_section && /^\[/ { exit }

    in_section {
      if ($1 ~ /^[A-Za-z0-9_.-]+=$/) {
        in_unflags = ($1 == "build_unflags=")
        next
      }

      if (in_unflags && $1 == "-D" && $2 == macro) {
        found = 1
        exit
      }
    }

    END { exit(found ? 0 : 1) }
  ' platformio.ini
}

if [[ -n "$target_env" ]]; then
  if [[ "$target_env" != env:* ]]; then
    target_env="env:${target_env}"
  fi

  if grep -Fq "[${target_env}]" platformio.ini; then
    ok "Scoped validation to environment: ${target_env}"
    env_list="${target_env}"
  else
    fail "Requested environment not found in platformio.ini: ${target_env}"
    env_list=""
  fi
else
  env_list="$(
    awk '
      /^\[env:[^]]+\]$/ {
        section = $0
        gsub(/^\[/, "", section)
        gsub(/\]$/, "", section)
        print section
      }
    ' platformio.ini
  )"
  if [[ -n "$env_list" ]]; then
    ok "No --env provided; validating all PlatformIO environments."
  fi
fi

env_count=0
while IFS= read -r env_name; do
  [[ -z "$env_name" ]] && continue
  env_count=$((env_count + 1))
  env_upper="$(echo "$env_name" | tr '[:lower:]' '[:upper:]')"
  if [[ "$env_upper" == *"DEBUG"* ]]; then
    ok "Skipping WEB_SECURE_ON enforcement for debug environment: ${env_name}"
    continue
  fi

  if section_has_unflag_macro "${env_name}" "WEB_SECURE_ON"; then
    if [[ "$env_upper" == *"TEST"* ]]; then
      warn "Test environment ${env_name} disables WEB_SECURE_ON."
    elif [[ "$env_upper" == *"RELEASE"* ]]; then
      if [[ $strict_release -eq 1 ]]; then
        fail "Release environment ${env_name} disables WEB_SECURE_ON via build_unflags."
      else
        warn "Release environment ${env_name} disables WEB_SECURE_ON via build_unflags."
      fi
    else
      warn "Non-debug environment ${env_name} disables WEB_SECURE_ON via build_unflags."
    fi
  else
    ok "Environment ${env_name} keeps WEB_SECURE_ON enabled."
  fi
done <<< "$env_list"

if [[ $env_count -eq 0 ]]; then
  fail "No [env:*] sections found in platformio.ini"
else
  ok "Detected ${env_count} PlatformIO environment(s)."
fi

constants_file="include/Constants.h"
if [[ ! -f "$constants_file" ]]; then
  fail "Missing ${constants_file}"
else
  config_url="$(awk -F'"' '/configUrl\{"/ { print $2; exit }' "$constants_file" || true)"
  ota_url="$(awk -F'"' '/otaUrl\{"/ { print $2; exit }' "$constants_file" || true)"

  if [[ -n "$config_url" ]]; then
    ok "Found CloudIO config URL: ${config_url}"
    if [[ "$config_url" == http://* ]]; then
      if [[ $fail_on_http -eq 1 ]]; then
        fail "CloudIO config URL uses http:// (${config_url})"
      else
        warn "CloudIO config URL uses http:// (${config_url})"
      fi
    fi
  else
    fail "Could not find CloudIO config URL in ${constants_file}"
  fi

  if [[ -n "$ota_url" ]]; then
    ok "Found OTA URL: ${ota_url}"
    if [[ "$ota_url" == http://* ]]; then
      if [[ $fail_on_http -eq 1 ]]; then
        fail "OTA URL uses http:// (${ota_url})"
      else
        warn "OTA URL uses http:// (${ota_url})"
      fi
    fi
  else
    fail "Could not find OTA URL in ${constants_file}"
  fi
fi

echo
echo "=============================================="
echo "Validation summary: ${failures} failure(s), ${warnings} warning(s)."
echo "=============================================="
echo

if [[ $failures -gt 0 ]]; then
  echo
  echo "Release metadata validation failed."
  echo
  exit 1
fi

echo
echo "Release metadata validation passed."
echo
