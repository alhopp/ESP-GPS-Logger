#!/usr/bin/env python3
"""Print first/last SBP frame indexes and speed in knots."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


SBP_HEADER_SIZE = 64
SBP_FRAME = struct.Struct("<BBHIIIiiHHhBB")
MMPS_TO_KNOTS = 0.0019438444924406


def frame_speed_knots(frame: bytes) -> float:
    fields = SBP_FRAME.unpack(frame)
    sog_cms = fields[8]
    return sog_cms * 10.0 * MMPS_TO_KNOTS


def find_latest_sbp() -> Path | None:
    candidates = sorted(
        Path(".").rglob("*.sbp"),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    return candidates[0] if candidates else None


def frame_count(path: Path) -> int:
    size = path.stat().st_size
    payload = max(0, size - SBP_HEADER_SIZE)
    return payload // SBP_FRAME.size


def read_edges(path: Path, edge_count: int) -> tuple[list[tuple[int, float]], list[tuple[int, float]]]:
    count = frame_count(path)
    if count <= 0:
        return [], []

    first_indexes = list(range(1, min(edge_count, count) + 1))
    last_start = max(1, count - edge_count + 1)
    last_indexes = list(range(last_start, count + 1))

    def read_rows(indexes: list[int]) -> list[tuple[int, float]]:
        rows = []
        with path.open("rb") as f:
            for index in indexes:
                f.seek(SBP_HEADER_SIZE + (index - 1) * SBP_FRAME.size)
                frame = f.read(SBP_FRAME.size)
                if len(frame) == SBP_FRAME.size:
                    rows.append((index, frame_speed_knots(frame)))
        return rows

    return read_rows(first_indexes), read_rows(last_indexes)


def print_rows(title: str, rows: list[tuple[int, float]]) -> None:
    print(title)
    print("index,speed_knots")
    for index, knots in rows:
        print(f"{index},{knots:.3f}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", nargs="?", help="SBP file path. Defaults to newest .sbp under this repo.")
    parser.add_argument("--count", type=int, default=5)
    args = parser.parse_args()

    path = Path(args.path) if args.path else find_latest_sbp()
    if not path:
        print("No .sbp file found. Download one into dev_sbp_logs/ first.")
        return 1

    first, last = read_edges(path, args.count)
    print(f"SBP: {path}")
    print(f"frames: {frame_count(path)}")
    print_rows("first", first)
    print_rows("last", last)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
