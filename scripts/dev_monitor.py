#!/usr/bin/env python3
"""Run PlatformIO monitor and auto-download SBP files in the background."""

from __future__ import annotations

import argparse
import subprocess
import threading
from pathlib import Path

import dev_sbp_autosync


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-url", default="http://gps.local")
    parser.add_argument("--out-dir", default="dev_sbp_logs")
    parser.add_argument("--poll-seconds", type=int, default=5)
    parser.add_argument("--baud", default="115200")
    args = parser.parse_args()

    sync_thread = threading.Thread(
        target=dev_sbp_autosync.watch,
        args=(args.base_url, Path(args.out_dir), args.poll_seconds),
        daemon=True,
    )
    sync_thread.start()

    print("[dev] starting PlatformIO monitor", flush=True)
    return subprocess.call(["pio", "device", "monitor", "--baud", args.baud])


if __name__ == "__main__":
    raise SystemExit(main())
