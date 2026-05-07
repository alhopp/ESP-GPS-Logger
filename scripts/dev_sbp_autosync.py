#!/usr/bin/env python3
"""Poll the ESP web API and download completed SBP sessions for dev checks."""

from __future__ import annotations

import argparse
import json
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path


def log(message: str) -> None:
    print(f"[SBP sync] {message}", flush=True)


def get_json(url: str) -> dict:
    with urllib.request.urlopen(url, timeout=4) as response:
        return json.loads(response.read().decode("utf-8"))


def download(url: str, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url, timeout=20) as response:
        target.write_bytes(response.read())


def download_name(file_info: dict) -> str | None:
    sbp_name = file_info.get("sbp_name")
    if sbp_name:
        return str(sbp_name)

    name = str(file_info.get("name", ""))
    if name.lower().endswith(".sbp"):
        return name

    return None


def sync_once(base_url: str, out_dir: Path) -> int:
    files_url = base_url.rstrip("/") + "/api/files"
    data = get_json(files_url)
    if not data.get("ok"):
        return 0

    sessions = []
    for file_info in data.get("files", []):
        name = download_name(file_info)
        if not name:
            continue
        sessions.append((int(file_info.get("mtime") or 0), name))

    downloaded = 0
    for _, name in sorted(sessions, reverse=True):
        target = out_dir / name
        if target.exists():
            continue

        encoded = urllib.parse.quote(name, safe="")
        url = base_url.rstrip("/") + f"/api/download?file={encoded}"
        download(url, target)
        downloaded += 1
        log(f"downloaded {name}")

    return downloaded


def watch(base_url: str, out_dir: Path, poll_seconds: int, once: bool = False) -> None:
    log(f"watching {base_url} -> {out_dir}")

    while True:
        try:
            sync_once(base_url, out_dir)
        except (urllib.error.URLError, TimeoutError, json.JSONDecodeError, OSError) as exc:
            log(f"waiting for device ({exc})")

        if once:
            return

        time.sleep(poll_seconds)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-url", default="http://gps.local")
    parser.add_argument("--out-dir", default="dev_sbp_logs")
    parser.add_argument("--poll-seconds", type=int, default=5)
    parser.add_argument("--once", action="store_true")
    args = parser.parse_args()

    watch(args.base_url, Path(args.out_dir), args.poll_seconds, args.once)


if __name__ == "__main__":
    main()
