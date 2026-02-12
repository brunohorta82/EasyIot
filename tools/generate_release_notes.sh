#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

usage() {
  cat <<'EOF'
Generate release notes draft from git commits.

Usage:
  tools/generate_release_notes.sh [--range <from..to>] [--count <n>] [--output <file>]

Options:
  --range, -r   Explicit git range (example: 523af13..HEAD)
  --count, -n   Number of recent commits when --range is not set (default: 15)
  --output, -o  Output markdown file (default: RELEASE_NOTES_DRAFT.md)
  --help, -h    Show this help
EOF
}

range=""
count=15
output_file="RELEASE_NOTES_DRAFT.md"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --range|-r)
      range="${2:-}"
      shift 2
      ;;
    --count|-n)
      count="${2:-}"
      shift 2
      ;;
    --output|-o)
      output_file="${2:-}"
      shift 2
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

if [[ -z "$range" ]]; then
  if git rev-parse --verify --quiet "HEAD~${count}" >/dev/null; then
    range="HEAD~${count}..HEAD"
  else
    oldest_commit="$(git rev-list --max-count="${count}" --reverse HEAD | awk 'NR==1{print $1}')"
    range="${oldest_commit}..HEAD"
  fi
fi

version="$(
  awk '
    $0 ~ /^\[extra\]/ { in_extra=1; next }
    /^\[/ { in_extra=0 }
    in_extra && $1=="version" && $2=="=" { print $3; exit }
  ' platformio.ini
)"
if [[ -z "$version" ]]; then
  version="unknown"
fi

declare -a commits=()
while IFS= read -r line; do
  commits+=("$line")
done < <(git log --no-merges --pretty=format:'%h|%s' "$range")

declare -a features=()
declare -a fixes=()
declare -a build_changes=()
declare -a docs_changes=()
declare -a other_changes=()

for commit in "${commits[@]}"; do
  hash="${commit%%|*}"
  subject="${commit#*|}"
  lowered_subject="$(printf '%s' "$subject" | tr '[:upper:]' '[:lower:]')"

  case "$lowered_subject" in
    feat:*|feat\(*|feature:*|feature\(*)
      features+=("${subject} (${hash})")
      ;;
    fix:*|fix\(*|hotfix:*|hotfix\(*)
      fixes+=("${subject} (${hash})")
      ;;
    build:*|build\(*|ci:*|ci\(*|chore:*|chore\(*|refactor:*|refactor\(*|perf:*|perf\(*)
      build_changes+=("${subject} (${hash})")
      ;;
    docs:*|docs\(*|doc:*|doc\(*)
      docs_changes+=("${subject} (${hash})")
      ;;
    *)
      other_changes+=("${subject} (${hash})")
      ;;
  esac
done

emit_section() {
  local title="$1"
  shift

  echo "## ${title}"
  if [[ $# -eq 0 ]]; then
    echo "- None"
  else
    for item in "$@"; do
      echo "- ${item}"
    done
  fi
  echo
}

generated_at="$(date '+%Y-%m-%d %H:%M:%S')"
current_branch="$(git rev-parse --abbrev-ref HEAD)"

{
  echo "# Release Notes Draft"
  echo
  echo "- Generated: ${generated_at}"
  echo "- Branch: ${current_branch}"
  echo "- Version: ${version}"
  echo "- Commit Range: ${range}"
  echo "- Commit Count: ${#commits[@]}"
  echo
  echo "## Summary"
  if [[ ${#commits[@]} -eq 0 ]]; then
    echo "- No commits found in the selected range."
  else
    echo "- Draft generated from git history. Review and edit before publishing."
  fi
  echo

  # Bash 3 + nounset can fail on empty array expansion; temporarily relax nounset here.
  set +u
  emit_section "Features" "${features[@]}"
  emit_section "Fixes" "${fixes[@]}"
  emit_section "Build / Infra" "${build_changes[@]}"
  emit_section "Docs" "${docs_changes[@]}"
  emit_section "Other Changes" "${other_changes[@]}"
  set -u

  echo "## Commit List"
  if [[ ${#commits[@]} -eq 0 ]]; then
    echo "- None"
  else
    for commit in "${commits[@]}"; do
      hash="${commit%%|*}"
      subject="${commit#*|}"
      echo "- ${hash} ${subject}"
    done
  fi
} > "${output_file}"

echo "Release notes draft written to ${output_file}"
