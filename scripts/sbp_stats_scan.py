#!/usr/bin/env python3
"""Scan an SBP file and print core windsurfing stats for comparison."""

from __future__ import annotations

import argparse
import datetime as dt
import math
import struct
from pathlib import Path


SBP_HEADER_SIZE = 64
SBP_FRAME = struct.Struct("<BBHIIiiiHHhBB")
MMPS_TO_KNOTS = 0.0019438444924406
SAMPLE_RATE_HZ = 5
ALPHA_RADIUS_M = 50.0
ALPHA_RADIUS_TOLERANCE_M = 1.0
ALPHA_SHORT_RADIUS_M = 49.85
ALPHA_SPEEDREADER_CLOSURE_TOLERANCE_M = 0.0
ALPHA_FULL_DISTANCE_TOLERANCE_START_M = 490.0
ALPHA_MIN_M = 100.0
ALPHA_MAX_M = 500.0
ALPHA_MAX_TIME_S = 194.0
FILTER_MIN_SPEED_KNOTS = 0.6
FILTER_MAX_ACCEL_MPS2 = 8.0
FILTER_MAX_HDOP = 5.0
FILTER_MIN_SATS = 5
MAX_PADDED_GAP_S = 20.0
NM_M = 1852.0
SPEED_DETECTION_MIN_MMPS = 4000.0
STANDSTILL_DETECTION_MAX_MMPS = 1000.0
MEAN_HEADING_TIME_S = 15
STRAIGHT_COURSE_MAX_DEG = 10.0
JIBE_COURSE_DEVIATION_MIN_DEG = 50.0
TIME_DELAY_NEW_RUN_S = 10


def read_sbp(path: Path, pad_missing: bool = False) -> list[dict[str, float]]:
    payload = path.read_bytes()[SBP_HEADER_SIZE:]
    rows: list[dict[str, float]] = []
    frame_bytes = len(payload) // SBP_FRAME.size * SBP_FRAME.size
    for offset in range(0, frame_bytes, SBP_FRAME.size):
        fields = SBP_FRAME.unpack(payload[offset : offset + SBP_FRAME.size])
        packed = fields[3]
        year_month = packed >> 22
        year = 2000 + (year_month - 1) // 12
        month = (year_month - 1) % 12 + 1
        day = (packed >> 17) & 0x1F
        hour = (packed >> 12) & 0x1F
        minute = (packed >> 6) & 0x3F
        second = packed & 0x3F
        millis = fields[2] % 1000
        timestamp = dt.datetime(year, month, day, hour, minute, second, millis * 1000)
        rows.append(
            {
                "row": len(rows) + 1,
                "lat": fields[5] * 1e-7,
                "lon": fields[6] * 1e-7,
                "speed_mmps": fields[8] * 10.0,
                "heading_deg": fields[9] / 100.0,
                "hdop": fields[0] / 10.0,
                "sats": fields[1],
                "time": timestamp,
                "synthetic": False,
            }
        )
    if pad_missing:
        rows = pad_missing_samples(rows)
    apply_speedreader_filters(rows)
    return rows


def pad_missing_samples(rows: list[dict[str, float]]) -> list[dict[str, float]]:
    if len(rows) < 2:
        return rows

    padded: list[dict[str, float]] = []
    sample_period_s = 1.0 / SAMPLE_RATE_HZ
    for index, row in enumerate(rows):
        if index == 0:
            padded.append(dict(row))
            continue

        previous = rows[index - 1]
        elapsed = (row["time"] - previous["time"]).total_seconds()
        missing = (
            max(0, round(elapsed / sample_period_s) - 1)
            if 0.0 < elapsed <= MAX_PADDED_GAP_S
            else 0
        )

        for missing_index in range(1, missing + 1):
            fraction = missing_index / (missing + 1)
            synthetic = dict(previous)
            synthetic["time"] = previous["time"] + dt.timedelta(seconds=sample_period_s * missing_index)
            synthetic["lat"] = previous["lat"] + (row["lat"] - previous["lat"]) * fraction
            synthetic["lon"] = previous["lon"] + (row["lon"] - previous["lon"]) * fraction
            synthetic["speed_mmps"] = 0.0
            synthetic["heading_deg"] = previous["heading_deg"]
            synthetic["hdop"] = max(previous["hdop"], row["hdop"])
            synthetic["sats"] = min(previous["sats"], row["sats"])
            synthetic["synthetic"] = True
            padded.append(synthetic)

        padded.append(dict(row))

    for row_index, row in enumerate(padded, start=1):
        row["row"] = row_index
    return padded


def apply_speedreader_filters(rows: list[dict[str, float]]) -> None:
    previous_mps: float | None = None
    previous_time: dt.datetime | None = None
    for row in rows:
        speed_mps = row["speed_mmps"] / 1000.0
        accel_ok = True
        if previous_mps is not None and previous_time is not None:
            elapsed = (row["time"] - previous_time).total_seconds()
            if elapsed > 0:
                accel_ok = abs(speed_mps - previous_mps) / elapsed <= FILTER_MAX_ACCEL_MPS2

        row["filtered"] = row.get("synthetic", False) or not (
            row["hdop"] <= FILTER_MAX_HDOP
            and row["sats"] >= FILTER_MIN_SATS
            and accel_ok
        )
        row["filtered_speed_mmps"] = 0.0 if row["filtered"] else row["speed_mmps"]
        previous_mps = speed_mps
        previous_time = row["time"]


class RunDetector:
    def __init__(self) -> None:
        self.old_heading = 0.0
        self.delta_heading = 0.0
        self.mean_heading = 0.0
        self.delay_counter = 0
        self.run_counter = 0
        self.velocity_0 = False
        self.velocity_5 = False
        self.straight_course = False
        self.alfa_counter = 0
        self.armed_count = 0
        self.jibe_count = 0
        self.standstill_count = 0
        self.last_armed_idx = -1
        self.last_jibe_idx = -1
        self.events: list[tuple[int, str, int]] = []

    def unwrap(self, actual_heading: float) -> float:
        if actual_heading - self.old_heading > 300.0:
            self.delta_heading -= 360.0
        if actual_heading - self.old_heading < -300.0:
            self.delta_heading += 360.0
        self.old_heading = actual_heading
        return actual_heading + self.delta_heading

    def update(self, idx: int, actual_heading: float, s2_speed_mmps: float) -> int:
        mean_samples = MEAN_HEADING_TIME_S * SAMPLE_RATE_HZ
        unwrapped_heading = self.unwrap(actual_heading)
        self.mean_heading = (
            self.mean_heading * (mean_samples - 1) / mean_samples
            + unwrapped_heading / mean_samples
        )
        heading_deviation = abs(self.mean_heading - unwrapped_heading)

        if s2_speed_mmps > SPEED_DETECTION_MIN_MMPS:
            self.velocity_5 = True
        if s2_speed_mmps < STANDSTILL_DETECTION_MAX_MMPS and self.velocity_5:
            self.velocity_0 = True

        if self.velocity_0 and s2_speed_mmps > SPEED_DETECTION_MIN_MMPS:
            self.velocity_5 = False
            self.velocity_0 = False
            self.delay_counter = (TIME_DELAY_NEW_RUN_S - 1) * SAMPLE_RATE_HZ
            self.standstill_count += 1
            self.events.append((idx, "standstill", self.run_counter))

        if (
            heading_deviation < STRAIGHT_COURSE_MAX_DEG
            and s2_speed_mmps > SPEED_DETECTION_MIN_MMPS
        ):
            if not self.straight_course:
                self.armed_count += 1
                self.last_armed_idx = idx
                self.events.append((idx, "armed", self.run_counter))
            self.straight_course = True

        if heading_deviation > JIBE_COURSE_DEVIATION_MIN_DEG and self.straight_course:
            self.straight_course = False
            self.delay_counter = 0
            self.alfa_counter += 1
            self.jibe_count += 1
            self.last_jibe_idx = idx
            self.events.append((idx, "jibe", self.run_counter))

        self.delay_counter += 1
        if self.delay_counter == TIME_DELAY_NEW_RUN_S * SAMPLE_RATE_HZ:
            self.run_counter += 1
            self.mean_heading = unwrapped_heading
            self.straight_course = s2_speed_mmps > SPEED_DETECTION_MIN_MMPS
            if self.straight_course:
                self.armed_count += 1
                self.last_armed_idx = idx
            self.events.append((idx, "run_start", self.run_counter))

        return self.run_counter


def detect_runs(rows: list[dict[str, float]]) -> tuple[list[int], RunDetector]:
    detector = RunDetector()
    runs: list[int] = []
    previous_2s_avg = 0.0
    rolling: list[float] = []

    for idx, row in enumerate(rows, start=1):
        runs.append(detector.update(idx, row["heading_deg"], previous_2s_avg))
        rolling.append(row["speed_mmps"])
        if len(rolling) > 2 * SAMPLE_RATE_HZ:
            rolling.pop(0)
        if len(rolling) == 2 * SAMPLE_RATE_HZ:
            previous_2s_avg = sum(rolling) / len(rolling)

    return runs, detector


def latest_sbp() -> Path | None:
    candidates = sorted(
        Path("devlogs").glob("*.sbp"),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    if candidates:
        return candidates[0]
    all_candidates = sorted(
        Path(".").rglob("*.sbp"),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    return all_candidates[0] if all_candidates else None


def closure_m(rows: list[dict[str, float]], start: int, end: int) -> float:
    lat0 = rows[start - 1]["lat"]
    lon0 = rows[start - 1]["lon"]
    lat1 = rows[end - 1]["lat"]
    lon1 = rows[end - 1]["lon"]
    dlat = lat1 - lat0
    dlon = (lon1 - lon0) * math.cos(math.radians(lat1))
    return math.sqrt(dlat * dlat + dlon * dlon) * 111195.0


def project_xy(rows: list[dict[str, float]]) -> list[tuple[float, float]]:
    if not rows:
        return []
    origin_lat = rows[0]["lat"]
    origin_lon = rows[0]["lon"]
    cos_lat = math.cos(math.radians(origin_lat))
    return [
        (
            (row["lon"] - origin_lon) * cos_lat * 111195.0,
            (row["lat"] - origin_lat) * 111195.0,
        )
        for row in rows
    ]


def proximity_m(
    xy: list[tuple[float, float]],
    start: int,
    end: int,
    start_offset: int = 0,
    end_offset: int = 0,
) -> float:
    start_index = max(0, min(len(xy) - 1, start - 1 + start_offset))
    end_index = max(0, min(len(xy) - 1, end - 1 + end_offset))
    x0, y0 = xy[start_index]
    x1, y1 = xy[end_index]
    return math.hypot(x1 - x0, y1 - y0)


def fixed_window_candidates(
    rows: list[dict[str, float]], samples: int
) -> list[tuple[float, int, int, float]]:
    if len(rows) < samples:
        return []

    total = sum(row["speed_mmps"] for row in rows[:samples])
    out = [(total / samples * MMPS_TO_KNOTS, 1, samples, total)]
    for right in range(samples, len(rows)):
        total += rows[right]["speed_mmps"] - rows[right - samples]["speed_mmps"]
        out.append((total / samples * MMPS_TO_KNOTS, right - samples + 2, right + 1, total))

    out.sort(reverse=True)
    return out


def best_fixed_window(rows: list[dict[str, float]], seconds: int) -> tuple[float, int, int, float]:
    candidates = fixed_window_candidates(rows, seconds * SAMPLE_RATE_HZ)
    return candidates[0] if candidates else (0.0, -1, -1, 0.0)


def top_non_overlapping_10s(rows: list[dict[str, float]], count: int) -> list[tuple[float, int, int, float]]:
    candidates = fixed_window_candidates(rows, 10 * SAMPLE_RATE_HZ)
    selected: list[tuple[float, int, int, float]] = []

    for candidate in candidates:
        _, start, end, _ = candidate
        overlaps = any(not (end < used_start or start > used_end) for _, used_start, used_end, _ in selected)
        if overlaps:
            continue
        selected.append(candidate)
        if len(selected) >= count:
            break

    return selected


def top_10s_per_run(
    rows: list[dict[str, float]], runs: list[int], count: int
) -> list[tuple[float, int, int, float, int]]:
    per_run: dict[int, tuple[float, int, int, float, int]] = {}

    for speed, start, end, total in fixed_window_candidates(rows, 10 * SAMPLE_RATE_HZ):
        run = runs[end - 1] if 1 <= end <= len(runs) else -1
        if run < 0:
            continue
        current = per_run.get(run)
        if current is None or speed > current[0]:
            per_run[run] = (speed, start, end, total, run)

    selected = sorted(per_run.values(), reverse=True)[:count]
    return selected


def distance_windows(
    rows: list[dict[str, float]], target_m: float, count: int
) -> list[tuple[float, int, int, float, int]]:
    target_scaled = target_m * 1000.0 * SAMPLE_RATE_HZ
    candidates: list[tuple[float, int, int, float, int]] = []
    left = 0
    distance_scaled = 0.0

    for right, row in enumerate(rows):
        distance_scaled += row["speed_mmps"]
        while distance_scaled > target_scaled and left < right:
            distance_scaled -= rows[left]["speed_mmps"]
            left += 1

        start = max(0, left - 1)
        scaled = sum(row["speed_mmps"] for row in rows[start : right + 1])
        samples = right - start + 1
        if scaled < target_scaled or samples <= 0:
            continue

        speed = scaled / samples * MMPS_TO_KNOTS
        candidates.append((speed, start + 1, right + 1, scaled / SAMPLE_RATE_HZ / 1000.0, samples))

    candidates.sort(reverse=True)
    selected: list[tuple[float, int, int, float, int]] = []
    for candidate in candidates:
        _, start, end, _, _ = candidate
        overlaps = any(not (end < used_start or start > used_end) for _, used_start, used_end, _, _ in selected)
        if overlaps:
            continue
        selected.append(candidate)
        if len(selected) >= count:
            break

    return selected


def alpha_windows(
    rows: list[dict[str, float]],
    count: int,
    radius_m: float = ALPHA_RADIUS_M,
    ignore_first_samples: int = 0,
    display_start_offset_at_boundary: bool = False,
) -> list[tuple[float, int, int, float, float, int, float, int]]:
    xy = project_xy(rows)
    cumulative_m = [0.0]
    cumulative_time_s = [0.0]
    cumulative_filtered = [0]

    for index, row in enumerate(rows):
        speed_mmps = row["speed_mmps"]
        if speed_mmps * MMPS_TO_KNOTS < FILTER_MIN_SPEED_KNOTS:
            speed_mmps = 0.0
        if index == 0:
            elapsed = 0.0
        else:
            elapsed = (row["time"] - rows[index - 1]["time"]).total_seconds()
            if elapsed <= 0 or elapsed > 1.0:
                elapsed = 1.0 / SAMPLE_RATE_HZ
        cumulative_m.append(cumulative_m[-1] + speed_mmps * elapsed / 1000.0)
        cumulative_time_s.append(cumulative_time_s[-1] + elapsed)
        cumulative_filtered.append(cumulative_filtered[-1] + (1 if row["filtered"] else 0))

    candidates: list[tuple[float, int, int, float, float, int, float, int]] = []
    for end in range(1, len(rows) + 1):
        for start in range(end - 1, ignore_first_samples, -1):
            distance_m = cumulative_m[end] - cumulative_m[start - 1]
            elapsed_s = (end - start + 1) / SAMPLE_RATE_HZ
            if elapsed_s <= 0:
                continue
            if elapsed_s > ALPHA_MAX_TIME_S:
                break
            if distance_m > ALPHA_MAX_M:
                break
            if distance_m < ALPHA_MIN_M:
                continue
            # GPS Speedreader appears to test the alpha circle from the
            # position sample immediately before the displayed start row.
            # A small tolerance matches Speedreader's rounded 50 m boundary.
            closure = proximity_m(xy, start, end, start_offset=-1)
            radius_limit = radius_m + ALPHA_SPEEDREADER_CLOSURE_TOLERANCE_M
            if closure > radius_limit:
                continue
            speed = distance_m / elapsed_s * 1.9438444924406
            display_start = start
            candidates.append((
                speed,
                int(rows[display_start - 1]["row"]),
                int(rows[end - 1]["row"]),
                distance_m,
                closure,
                end - display_start + 1,
                elapsed_s,
                cumulative_filtered[end] - cumulative_filtered[start - 1],
            ))

    candidates.sort(reverse=True)
    return candidates if count <= 0 else candidates[:count]


def select_non_overlapping_alpha(
    candidates: list[tuple[float, int, int, float, float, int, float, int]],
    count: int,
) -> list[tuple[float, int, int, float, float, int, float, int]]:
    selected: list[tuple[float, int, int, float, float, int, float, int]] = []
    for candidate in candidates:
        _, start, end, _, _, _, _, _ = candidate
        overlaps = any(
            not (end < used_start or start > used_end)
            for _, used_start, used_end, _, _, _, _, _ in selected
        )
        if overlaps:
            continue
        selected.append(candidate)
        if len(selected) >= count:
            break
    return selected


def print_stats(path: Path, start_row: int = 1, end_row: int | None = None) -> None:
    rows = read_sbp(path)
    original_count = len(rows)
    if start_row < 1:
        start_row = 1
    if end_row is None or end_row > len(rows):
        end_row = len(rows)
    rows = rows[start_row - 1 : end_row]
    runs, detector = detect_runs(rows)
    total_m = sum(row["speed_mmps"] for row in rows) / SAMPLE_RATE_HZ / 1000.0
    two_s = best_fixed_window(rows, 2)
    top_10s = top_10s_per_run(rows, runs, 5)
    nm_windows = distance_windows(rows, NM_M, 5)
    ignore_alpha_start = 1 if start_row > 1 else 0
    display_boundary_offset = start_row > 1
    alpha_candidates = alpha_windows(rows, 0, 50.0, ignore_alpha_start, display_boundary_offset)
    alpha_results = select_non_overlapping_alpha(alpha_candidates, 5)
    alpha_raw_results = alpha_candidates[:10]
    alpha_60_results = select_non_overlapping_alpha(
        alpha_windows(rows, 0, 60.0, ignore_alpha_start, display_boundary_offset), 5
    )
    alpha_70_results = select_non_overlapping_alpha(
        alpha_windows(rows, 0, 70.0, ignore_alpha_start, display_boundary_offset), 5
    )
    h1 = total_m / 3600.0 * 1000.0 * MMPS_TO_KNOTS
    avg_10s = sum(row[0] for row in top_10s) / 5.0 if len(top_10s) == 5 else 0.0

    print(f"SBP: {path}")
    if start_row != 1 or end_row != original_count:
        print(f"rows: {start_row} -> {end_row} of {original_count}")
    print(f"frames: {len(rows)}")
    print()
    print(f"2s:     {two_s[0]:.3f} kn  {two_s[1]} -> {two_s[2]}")
    print("10s top 5 per run:")
    for index in range(5):
        if index < len(top_10s):
            speed, start, end, _, run = top_10s[index]
            print(f"  #{index + 1}:  {speed:.3f} kn  run={run}  {start} -> {end}")
        else:
            print(f"  #{index + 1}:  0.000 kn  n/a")
    print(f"10s avg: {avg_10s:.3f} kn")
    print()
    print("Run detector:")
    print(f"  run count:   {detector.run_counter}")
    print(f"  armed:       {detector.armed_count} last={detector.last_armed_idx}")
    print(f"  jibes:       {detector.jibe_count} last={detector.last_jibe_idx}")
    print(f"  standstill:  {detector.standstill_count}")
    print("NM top 5:")
    for index in range(5):
        if index < len(nm_windows):
            speed, start, end, distance_m, samples = nm_windows[index]
            print(f"  #{index + 1}:  {speed:.3f} kn  {start} -> {end}  dist={distance_m:.1f}m samples={samples}")
        else:
            print(f"  #{index + 1}:  0.000 kn  n/a")
    def print_alpha_table(title: str, results: list[tuple[float, int, int, float, float, int, float, int]]) -> None:
        print(title)
        for index in range(len(results) if len(results) > 5 else 5):
            if index < len(results):
                speed, start, end, distance_m, closure, samples, elapsed_s, filtered_count = results[index]
                print(
                    f"  #{index + 1}:  {speed:.3f} kn  {start} -> {end}  "
                    f"dist={distance_m:.1f}m closure={closure:.1f}m "
                    f"time={elapsed_s:.1f}s samples={samples} filtered={filtered_count}"
                )
            else:
                print(f"  #{index + 1}:  0.000 kn  n/a")

    print_alpha_table("Alpha 50m top 5:", alpha_results)
    print_alpha_table("Alpha 50m raw top 10:", alpha_raw_results)
    print_alpha_table("Alpha 60m top 5:", alpha_60_results)
    print_alpha_table("Alpha 70m top 5:", alpha_70_results)
    print(f"1h pad:  {h1:.3f} kn")
    print(f"Dist:    {total_m / 1000.0:.3f} km  1 -> {len(rows)}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", nargs="?", help="SBP file path. Defaults to newest devlogs/*.sbp.")
    parser.add_argument("--from", dest="start_row", type=int, default=1, help="1-based first SBP row to include.")
    parser.add_argument("--to", dest="end_row", type=int, default=None, help="1-based last SBP row to include.")
    args = parser.parse_args()

    path = Path(args.path) if args.path else latest_sbp()
    if not path:
        print("No SBP file found.")
        return 1

    print_stats(path, args.start_row, args.end_row)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
