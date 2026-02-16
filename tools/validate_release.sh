#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if command -v python3 >/dev/null 2>&1; then
  PY=python3
elif command -v python >/dev/null 2>&1; then
  PY=python
else
  echo "Python is required to run tools/validate_release.py" >&2
  exit 1
fi

exec "$PY" "$ROOT/tools/validate_release.py" "$@"

