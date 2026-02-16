#!/usr/bin/env python3
"""
Cross-platform release notes draft generator.

Python equivalent of generate_release_notes.sh for Windows/macOS/Linux.
"""

from __future__ import annotations

import argparse
import datetime as dt
import re
import subprocess
import sys
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple


ROOT = Path(__file__).resolve().parent.parent
PLATFORMIO_INI = ROOT / "platformio.ini"


def _run_git(args: Sequence[str]) -> str:
    proc = subprocess.run(
        ["git", *args],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return proc.stdout.strip()


def _strip_inline_comment(line: str) -> str:
    for mark in (";", "#"):
        idx = line.find(mark)
        if idx != -1:
            line = line[:idx]
    return line.rstrip()


def _read_extra_version(lines: Iterable[str]) -> str:
    in_extra = False
    for raw in lines:
        line = raw.strip()
        if line == "[extra]":
            in_extra = True
            continue
        if line.startswith("["):
            in_extra = False
        if not in_extra:
            continue
        clean = _strip_inline_comment(raw).strip()
        m = re.match(r"^version\s*=\s*(.+)$", clean)
        if m:
            return m.group(1).strip()
    return "unknown"


def _categorize(subject: str) -> str:
    lowered = subject.lower()
    if re.match(r"^(feat|feature)(:|\()", lowered):
        return "features"
    if re.match(r"^(fix|hotfix)(:|\()", lowered):
        return "fixes"
    if re.match(r"^(build|ci|chore|refactor|perf)(:|\()", lowered):
        return "build"
    if re.match(r"^(docs|doc)(:|\()", lowered):
        return "docs"
    return "other"


def _emit_section(title: str, items: List[str]) -> List[str]:
    out = [f"## {title}"]
    if not items:
        out.append("- None")
    else:
        out.extend(f"- {item}" for item in items)
    out.append("")
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate release notes draft from git commits.")
    parser.add_argument("--range", "-r", dest="commit_range", default="", help="Explicit git range (<from..to>)")
    parser.add_argument("--count", "-n", type=int, default=15, help="Recent commit count when --range is not set")
    parser.add_argument("--output", "-o", default="RELEASE_NOTES_DRAFT.md", help="Output markdown file")
    args = parser.parse_args()

    commit_range = args.commit_range.strip()
    count = max(1, args.count)
    output_file = ROOT / args.output

    if not commit_range:
        # If HEAD~count exists use that; otherwise use oldest from available recent commits.
        rev_ok = subprocess.run(
            ["git", "rev-parse", "--verify", "--quiet", f"HEAD~{count}"],
            cwd=ROOT,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        ).returncode == 0

        if rev_ok:
            commit_range = f"HEAD~{count}..HEAD"
        else:
            oldest = _run_git(["rev-list", f"--max-count={count}", "--reverse", "HEAD"]).splitlines()
            oldest_commit = oldest[0] if oldest else "HEAD"
            commit_range = f"{oldest_commit}..HEAD"

    pio_lines = PLATFORMIO_INI.read_text(encoding="utf-8").splitlines() if PLATFORMIO_INI.exists() else []
    version = _read_extra_version(pio_lines)

    raw_commits = _run_git(["log", "--no-merges", "--pretty=format:%h|%s", commit_range])
    commits: List[Tuple[str, str]] = []
    if raw_commits:
        for line in raw_commits.splitlines():
            if "|" not in line:
                continue
            h, s = line.split("|", 1)
            commits.append((h, s))

    features: List[str] = []
    fixes: List[str] = []
    build_changes: List[str] = []
    docs_changes: List[str] = []
    other_changes: List[str] = []

    for h, s in commits:
        entry = f"{s} ({h})"
        category = _categorize(s)
        if category == "features":
            features.append(entry)
        elif category == "fixes":
            fixes.append(entry)
        elif category == "build":
            build_changes.append(entry)
        elif category == "docs":
            docs_changes.append(entry)
        else:
            other_changes.append(entry)

    generated_at = dt.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    current_branch = _run_git(["rev-parse", "--abbrev-ref", "HEAD"])

    lines: List[str] = []
    lines.append("# Release Notes Draft")
    lines.append("")
    lines.append(f"- Generated: {generated_at}")
    lines.append(f"- Branch: {current_branch}")
    lines.append(f"- Version: {version}")
    lines.append(f"- Commit Range: {commit_range}")
    lines.append(f"- Commit Count: {len(commits)}")
    lines.append("")
    lines.append("## Summary")
    if not commits:
        lines.append("- No commits found in the selected range.")
    else:
        lines.append("- Draft generated from git history. Review and edit before publishing.")
    lines.append("")

    lines.extend(_emit_section("Features", features))
    lines.extend(_emit_section("Fixes", fixes))
    lines.extend(_emit_section("Build / Infra", build_changes))
    lines.extend(_emit_section("Docs", docs_changes))
    lines.extend(_emit_section("Other Changes", other_changes))

    lines.append("## Commit List")
    if not commits:
        lines.append("- None")
    else:
        for h, s in commits:
            lines.append(f"- {h} {s}")

    output_file.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Release notes draft written to {output_file}")
    print("")
    return 0


if __name__ == "__main__":
    sys.exit(main())

