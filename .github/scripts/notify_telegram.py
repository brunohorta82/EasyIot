#!/usr/bin/env python3
"""Announce a deploy on the Telegram channel.

The message has to say what changed, not that something changed: "new version
deployed" makes a tester open the app to find out, which is the work the message
was supposed to save.

Where the repo keeps a CHANGELOG, the bullets there are written for developers and
run several lines each. Only the lead sentence of each bullet is sent — in these
changelogs that is the bolded part, which is already a plain-language summary.
Repos without a changelog fall back to commit subjects since the previous tag.

Reads from the environment: TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID, PRODUCT,
VERSION, optionally CHANGELOG_FILE and CHANGELOG_VERSION.
"""
import html
import os
import re
import subprocess
import sys
import urllib.parse
import urllib.request

MAX_BULLETS = 12
MAX_BULLET_CHARS = 220


def changelog_section(path, version):
    if not path or not os.path.exists(path):
        return []
    text = open(path, encoding="utf-8").read().splitlines()
    start = re.compile(r"^## \[?" + re.escape(version) + r"\]?")
    inside, block = False, []
    for line in text:
        if start.match(line):
            inside = True
            continue
        if inside and line.startswith("## "):
            break
        if inside:
            block.append(line)
    return block


TESTERS_HEADING = "### Para testers"


def testers_subsection(block):
    """The channel is read by testers, not by whoever wrote the code, and the
    changelog is written in English for the repository. A release can therefore
    carry a short Portuguese section aimed at the channel; when it does, that is
    the whole message. It is kept in the same file so it cannot drift away from
    the entry it describes."""
    out, inside = [], False
    for line in block:
        if line.strip().startswith("### "):
            inside = line.strip() == TESTERS_HEADING
            continue
        if inside:
            out.append(line)
    return out


def bullets_from(block):
    """One entry per '- ' item, joining the lines it wraps onto."""
    items, current = [], None
    for line in block:
        stripped = line.strip()
        if stripped.startswith("- "):
            if current:
                items.append(current)
            current = stripped[2:]
        elif stripped.startswith("#") or not stripped:
            continue
        elif current is not None:
            current += " " + stripped
    if current:
        items.append(current)

    out = []
    for item in items:
        # The bolded lead is the human summary; the rest is the reasoning.
        lead = re.match(r"\*\*(.+?)\*\*", item)
        text = lead.group(1) if lead else item.split(". ")[0]
        text = text.replace("**", "").replace("`", "").strip()
        if len(text) > MAX_BULLET_CHARS:
            text = text[: MAX_BULLET_CHARS - 1].rstrip() + "…"
        if text:
            out.append(text)
    return out[:MAX_BULLETS]


def commit_subjects():
    def run(args):
        return subprocess.run(args, capture_output=True, text=True).stdout.strip()

    prev = run(["git", "describe", "--tags", "--abbrev=0", "HEAD^"])
    rng = [f"{prev}..HEAD"] if prev else ["-12"]
    log = run(["git", "log", "--no-merges", "--pretty=%s"] + rng)
    return [l for l in log.splitlines() if l][:MAX_BULLETS]


def main():
    token = os.environ.get("TELEGRAM_BOT_TOKEN", "")
    chat = os.environ.get("TELEGRAM_CHAT_ID", "")
    product = os.environ.get("PRODUCT", "")
    version = os.environ.get("VERSION", "")
    if not product or not version:
        print("::error::PRODUCT and VERSION are required")
        return 1
    if not token or not chat:
        print("::warning::Telegram secrets are not set — skipping the announcement")
        return 0

    section = changelog_section(
        os.environ.get("CHANGELOG_FILE", "CHANGELOG.md"),
        os.environ.get("CHANGELOG_VERSION", version),
    )
    lines = bullets_from(testers_subsection(section)) or bullets_from(section)
    if not lines:
        lines = commit_subjects()
    if not lines:
        lines = ["Sem notas para esta versão."]

    body = "\n".join("• " + html.escape(l) for l in lines)
    text = f"<b>{html.escape(product)} {html.escape(version)}</b>\n{body}"

    data = urllib.parse.urlencode({
        "chat_id": chat,
        "text": text,
        "parse_mode": "HTML",
        "disable_web_page_preview": "true",
    }).encode()
    req = urllib.request.Request(
        f"https://api.telegram.org/bot{token}/sendMessage", data=data)
    try:
        with urllib.request.urlopen(req, timeout=20) as r:
            r.read()
    except Exception as e:
        # Never fatal: a release that shipped but was not announced still shipped,
        # and failing the job here would say otherwise.
        detail = getattr(e, "read", lambda: b"")()[:300].decode("utf-8", "replace")
        print(f"::warning::Telegram refused the announcement: {e} {detail}")
        return 0
    print(f"Announced {product} {version} on Telegram")
    return 0


if __name__ == "__main__":
    sys.exit(main())
