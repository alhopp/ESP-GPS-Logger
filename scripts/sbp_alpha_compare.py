#!/usr/bin/env python3
"""Compare alpha-window variants against known GPS Speedreader results."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import sys
from typing import Iterable

import sbp_stats_scan as sbp


@dataclass(frozen=True)
class Truth:
    rank: int
    speed_kn: float
    start_row: int | None = None
    end_row: int | None = None
    end_time: str | None = None


@dataclass(frozen=True)
class Case:
    name: str
    path: Path
    start_row: int = 1
    end_row: int | None = None
    speedreader_utc_offset_hours: int = 8
    truth: dict[int, tuple[Truth, ...]] | None = None


@dataclass(frozen=True)
class Variant:
    name: str
    speed_mode: str = "rows"
    distance_mode: str = "speed"
    elapsed_mode: str = "samples"
    filter_mode: str = "low_zero"
    closure_start_offset: int = 0
    closure_end_offset: int = 0
    closure_offsets: tuple[tuple[int, int], ...] | None = None
    near_full_closure_offsets: tuple[tuple[int, int], ...] | None = None
    full_tolerance_m: float = 1.0
    short_tolerance_m: float = -0.15
    min_distance_m: float = 200.0
    min_distance_radius_factor: float = 0.0
    low_speed_kn: float = sbp.FILTER_MIN_SPEED_KNOTS


@dataclass(frozen=True)
class Candidate:
    speed_kn: float
    start_row: int
    end_row: int
    distance_m: float
    closure_m: float
    samples: int
    elapsed_s: float
    filtered: int
    max_radius_m: float = 0.0


CASES = (
    Case(
        "2026-01-13",
        Path("devlogs/2026-01-13_S01_BBBC2C.sbp"),
        truth={
            50: (
                Truth(1, 28.976, end_time="15:02:58.000"),
                Truth(2, 27.598, end_time="15:07:02.400"),
                Truth(3, 27.262, end_time="15:06:35.600"),
                Truth(4, 27.013, end_time="15:05:27.000"),
                Truth(5, 23.424, end_time="15:03:57.000"),
            ),
            60: (
                Truth(1, 33.234, end_time="15:02:52.800"),
                Truth(2, 30.263, end_time="15:06:59.600"),
                Truth(3, 27.281, end_time="15:06:35.800"),
                Truth(4, 26.872, end_time="15:05:27.600"),
                Truth(5, 23.424, end_time="15:03:57.000"),
            ),
            70: (
                Truth(1, 34.555, end_time="15:02:52.200"),
                Truth(2, 31.951, end_time="15:06:58.400"),
                Truth(3, 27.300, end_time="15:06:36.000"),
                Truth(4, 27.050, end_time="15:05:28.000"),
                Truth(5, 23.446, end_time="15:01:29.600"),
            ),
        },
    ),
    Case(
        "2026-01-16",
        Path("devlogs/2026-01-16_S01_BBBC2C.sbp"),
        truth={
            50: (
                Truth(1, 34.857, end_time="18:19:12.600"),
                Truth(2, 28.052, end_time="18:17:57.800"),
                Truth(3, 22.887, end_time="18:20:58.800"),
                Truth(4, 21.628, end_time="18:17:16.400"),
            ),
            60: (
                Truth(1, 35.612, end_time="18:19:12.800"),
                Truth(2, 28.942, end_time="18:16:00.800"),
                Truth(3, 28.395, end_time="18:17:42.000"),
                Truth(4, 23.383, end_time="18:20:59.200"),
                Truth(5, 21.672, end_time="18:17:16.000"),
            ),
            70: (
                Truth(1, 35.974, end_time="18:19:13.000"),
                Truth(2, 30.266, end_time="18:17:41.200"),
                Truth(3, 29.962, end_time="18:16:00.400"),
                Truth(4, 23.411, end_time="18:21:00.000"),
                Truth(5, 21.672, end_time="18:17:16.000"),
            ),
        },
    ),
    Case(
        "2026-06-07",
        Path("devlogs/2026-06-07_S01_BBBC2C.sbp"),
        truth={
            50: (
                Truth(1, 36.666, 1295, 1363, "18:13:47.000"),
                Truth(2, 36.339, end_time="18:13:22.600"),
                Truth(3, 36.015, 4140, 4195, "18:23:13.400"),
                Truth(4, 35.174, end_time="18:14:16.200"),
                Truth(5, 35.096, end_time="18:18:25.600"),
            ),
            60: (
                Truth(1, 36.681, end_time="18:13:42.200"),
                Truth(2, 36.419, end_time="18:13:22.000"),
                Truth(3, 36.224, end_time="18:23:12.600"),
                Truth(4, 35.761, end_time="18:11:10.800"),
                Truth(5, 35.228, end_time="18:14:13.200"),
            ),
            70: (
                Truth(1, 36.702, end_time="18:13:53.200"),
                Truth(2, 36.475, end_time="18:13:21.600"),
                Truth(3, 36.420, end_time="18:23:10.400"),
                Truth(4, 35.761, end_time="18:11:10.800"),
                Truth(5, 35.405, end_time="18:14:13.400"),
            ),
        },
    ),
    Case(
        "2026-07-21",
        Path("devlogs/2026-07-21_S01_BBBC2C.sbp"),
        truth={
            50: (
                Truth(1, 36.221, end_time="20:15:17.600"),
                Truth(2, 30.637, end_time="20:16:26.400"),
                Truth(3, 28.851, end_time="20:12:02.000"),
                Truth(4, 28.383, end_time="20:15:50.200"),
                Truth(5, 24.920, end_time="20:09:42.800"),
            ),
            60: (
                Truth(1, 36.658, end_time="20:15:17.800"),
                Truth(2, 30.551, end_time="20:16:26.600"),
                Truth(3, 29.922, end_time="20:12:02.000"),
                Truth(4, 29.074, end_time="20:15:50.400"),
                Truth(5, 26.311, end_time="20:12:49.400"),
            ),
            70: (
                Truth(1, 36.818, end_time="20:15:18.000"),
                Truth(2, 30.441, end_time="20:16:26.800"),
                Truth(3, 30.403, end_time="20:04:50.600"),
                Truth(4, 30.381, end_time="20:12:02.200"),
                Truth(5, 29.615, end_time="20:15:50.400"),
            ),
        },
    ),
    Case(
        "2026-09-04",
        Path("devlogs/2026-09-04_S01_BBBC2C.sbp"),
        truth={
            50: (
                Truth(1, 33.949, end_time="01:05:12.400"),
                Truth(2, 23.745, end_time="01:03:36.600"),
                Truth(3, 21.790, end_time="01:07:39.400"),
            ),
            60: (
                Truth(1, 34.853, end_time="01:05:12.000"),
                Truth(2, 25.653, end_time="01:03:16.400"),
                Truth(3, 24.481, end_time="01:08:18.600"),
                Truth(4, 21.802, end_time="01:07:38.600"),
            ),
            70: (
                Truth(1, 35.273, end_time="01:05:12.200"),
                Truth(2, 27.995, end_time="01:03:15.000"),
                Truth(3, 25.724, end_time="01:08:18.400"),
                Truth(4, 21.852, end_time="01:07:38.200"),
            ),
        },
    ),
    Case(
        "2026-11-05",
        Path("devlogs/2026-11-05_S01_BBBC2C.sbp"),
        truth={
            50: (
                Truth(1, 34.959, end_time="18:47:49.200"),
                Truth(2, 32.804, end_time="18:41:03.600"),
                Truth(3, 28.386, end_time="18:34:33.600"),
                Truth(4, 23.904, end_time="18:45:02.600"),
                Truth(5, 23.462, end_time="18:46:47.600"),
            ),
            60: (
                Truth(1, 34.735, end_time="18:47:49.600"),
                Truth(2, 33.747, end_time="18:41:03.600"),
                Truth(3, 30.392, end_time="18:37:11.400"),
                Truth(4, 29.249, end_time="18:34:33.400"),
                Truth(5, 28.954, end_time="18:44:40.200"),
            ),
            70: (
                Truth(1, 34.704, end_time="18:47:49.800"),
                Truth(2, 34.375, end_time="18:41:03.800"),
                Truth(3, 31.956, end_time="18:36:28.200"),
                Truth(4, 30.865, end_time="18:37:11.800"),
                Truth(5, 30.019, end_time="18:44:40.000"),
            ),
        },
    ),
    Case(
        "Suncombo",
        Path("devlogs/Suncombo_exp (1).sbp"),
        truth={
            50: (
                Truth(1, 20.319, 174, 412),
                Truth(2, 9.871, 16556, 17047),
                Truth(3, 7.829, 32390, 32648),
                Truth(4, 7.061, 11455, 11739),
                Truth(5, 7.032, 2246, 2622),
            ),
            60: (
                Truth(1, 20.398),
                Truth(2, 9.882),
                Truth(3, 8.510),
                Truth(4, 8.059),
                Truth(5, 7.771),
            ),
            70: (
                Truth(1, 20.475),
                Truth(2, 9.908),
                Truth(3, 9.516),
                Truth(4, 9.139),
                Truth(5, 8.338),
            ),
        },
    ),
    Case(
        "ALGPS Jan5",
        Path("devlogs/ALGPS_168601046_20230105_222331 (7).sbp"),
        start_row=35884,
        truth={
            50: (
                Truth(1, 21.618, 100393, 100616),
                Truth(2, 21.405, 99330, 99556),
                Truth(3, 20.534, 40235, 40470),
                Truth(4, 19.129, 35885, 36102),
                Truth(5, 18.686, 93028, 93286),
            ),
            60: (
                Truth(1, 23.592),
                Truth(2, 21.645),
                Truth(3, 21.419),
                Truth(4, 20.553),
                Truth(5, 19.304),
            ),
            70: (
                Truth(1, 23.622),
                Truth(2, 21.673),
                Truth(3, 21.440),
                Truth(4, 20.580),
                Truth(5, 19.431),
            ),
        },
    ),
)


VARIANTS = (
    Variant("current"),
    Variant("strict50", full_tolerance_m=0.0, short_tolerance_m=0.0),
    Variant("prev-end-closure", closure_end_offset=-1, full_tolerance_m=0.0, short_tolerance_m=0.0),
    Variant("prev-end-near500", closure_end_offset=-1, full_tolerance_m=1.0, short_tolerance_m=0.0),
    Variant("next-end-closure", closure_end_offset=1, full_tolerance_m=0.0, short_tolerance_m=0.0),
    Variant("prev-start-prev-end", closure_start_offset=-1, closure_end_offset=-1, full_tolerance_m=0.0, short_tolerance_m=0.0),
    Variant("prev-start-end", closure_start_offset=-1, closure_end_offset=0, full_tolerance_m=0.0, short_tolerance_m=0.0),
    Variant(
        "boundary-min",
        closure_offsets=((-1, -1), (-1, 0), (0, -1), (0, 0)),
        full_tolerance_m=0.0,
        short_tolerance_m=0.0,
    ),
    Variant(
        "hybrid-full-prev-start",
        closure_offsets=((0, 0),),
        near_full_closure_offsets=((-1, 0),),
        full_tolerance_m=0.0,
        short_tolerance_m=0.0,
    ),
    Variant(
        "speedreader-prev-start",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=0.2,
        short_tolerance_m=0.2,
    ),
    Variant(
        "speedreader-dynamic-min",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=0.2,
        short_tolerance_m=0.2,
        min_distance_m=100.0,
        min_distance_radius_factor=2.0,
    ),
    Variant(
        "speedreader-dynamic-strict",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=0.0,
        short_tolerance_m=0.0,
        min_distance_m=100.0,
        min_distance_radius_factor=2.0,
    ),
    Variant(
        "speedreader-dynamic-filtered-zero",
        filter_mode="filtered_zero",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=0.0,
        short_tolerance_m=0.0,
        min_distance_m=100.0,
        min_distance_radius_factor=2.0,
    ),
    Variant(
        "speedreader-dynamic-49p5",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=-0.5,
        short_tolerance_m=-0.5,
        min_distance_m=100.0,
        min_distance_radius_factor=2.0,
    ),
    Variant("start-next-end", closure_start_offset=0, closure_end_offset=1, full_tolerance_m=0.0, short_tolerance_m=0.0),
    Variant("next-start-end", closure_start_offset=1, closure_end_offset=0, full_tolerance_m=0.0, short_tolerance_m=0.0),
    Variant("interval-speed", speed_mode="into", elapsed_mode="intervals"),
    Variant(
        "speedreader-prev-start-interval-rows",
        elapsed_mode="intervals",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=0.2,
        short_tolerance_m=0.2,
    ),
    Variant(
        "speedreader-prev-start-interval-into",
        speed_mode="into",
        elapsed_mode="intervals",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=0.2,
        short_tolerance_m=0.2,
    ),
    Variant(
        "speedreader-prev-start-interval-outof",
        speed_mode="outof",
        elapsed_mode="intervals",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=0.2,
        short_tolerance_m=0.2,
    ),
    Variant("position-distance", distance_mode="position", full_tolerance_m=0.0, short_tolerance_m=0.0),
    Variant("no-min-current", min_distance_m=0.0),
    Variant(
        "speedreader-prev-start-no-low-zero",
        filter_mode="none",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=0.2,
        short_tolerance_m=0.2,
    ),
    Variant(
        "speedreader-prev-start-pos-low-zero",
        filter_mode="pos_low_zero",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=0.2,
        short_tolerance_m=0.2,
    ),
    Variant(
        "speedreader70-real",
        filter_mode="pos_low_zero",
        closure_start_offset=-1,
        closure_end_offset=0,
        full_tolerance_m=0.2,
        short_tolerance_m=0.2,
        low_speed_kn=0.545,
    ),
)


def load_case(case: Case) -> list[dict[str, float]]:
    rows = sbp.read_sbp(case.path, pad_missing=True)
    end_row = case.end_row if case.end_row is not None else len(rows)
    return rows[case.start_row - 1 : end_row]


def row_speed_mmps(row: dict[str, float], mode: str, low_speed_kn: float) -> float:
    speed = float(row["speed_mmps"])
    if mode in ("low_zero", "filtered_zero") and speed * sbp.MMPS_TO_KNOTS < low_speed_kn:
        return 0.0
    if mode == "filtered_zero" and row.get("filtered", False):
        return 0.0
    return speed


def row_speed_values(
    rows: list[dict[str, float]],
    xy: list[tuple[float, float]],
    variant: Variant,
) -> list[float]:
    values: list[float] = []
    for index, row in enumerate(rows):
        speed = float(row["speed_mmps"])
        if variant.filter_mode == "pos_low_zero":
            if index == 0:
                position_knots = speed * sbp.MMPS_TO_KNOTS
            else:
                x0, y0 = xy[index - 1]
                x1, y1 = xy[index]
                position_mps = ((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5 * sbp.SAMPLE_RATE_HZ
                position_knots = position_mps * 1.9438444924406
            if position_knots < variant.low_speed_kn:
                speed = 0.0
        else:
            speed = row_speed_mmps(row, variant.filter_mode, variant.low_speed_kn)
        values.append(speed)
    return values


def prefix(values: Iterable[float]) -> list[float]:
    out = [0.0]
    for value in values:
        out.append(out[-1] + value)
    return out


def range_sum(pref: list[float], start: int, end: int) -> float:
    if end < start:
        return 0.0
    return pref[end + 1] - pref[start]


def speed_bounds(start: int, end: int, mode: str) -> tuple[int, int, int]:
    if mode == "rows":
        return start, end, end - start + 1
    if mode == "into":
        return start + 1, end, max(0, end - start)
    if mode == "outof":
        return start, end - 1, max(0, end - start)
    raise ValueError(f"unknown speed mode {mode}")


def elapsed_s(start: int, end: int, variant: Variant) -> float:
    if variant.elapsed_mode == "samples":
        return (end - start + 1) / sbp.SAMPLE_RATE_HZ
    if variant.elapsed_mode == "intervals":
        return max(0, end - start) / sbp.SAMPLE_RATE_HZ
    raise ValueError(f"unknown elapsed mode {variant.elapsed_mode}")


def build_segment_prefix(rows: list[dict[str, float]], xy: list[tuple[float, float]]) -> list[float]:
    segments = [0.0]
    for index in range(1, len(rows)):
        x0, y0 = xy[index - 1]
        x1, y1 = xy[index]
        segments.append(((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5)
    return prefix(segments)


def candidate_distance_m(
    speed_prefix: list[float],
    position_prefix: list[float],
    start: int,
    end: int,
    variant: Variant,
) -> float:
    if variant.distance_mode == "position":
        return range_sum(position_prefix, start + 1, end)
    lo, hi, _ = speed_bounds(start, end, variant.speed_mode)
    return range_sum(speed_prefix, lo, hi) / 1000.0 / sbp.SAMPLE_RATE_HZ


def candidate_speed_kn(speed_prefix: list[float], start: int, end: int, variant: Variant) -> float:
    lo, hi, denom = speed_bounds(start, end, variant.speed_mode)
    if variant.elapsed_mode == "intervals" and variant.speed_mode == "rows":
        denom = max(0, end - start)
    if denom <= 0:
        return 0.0
    return range_sum(speed_prefix, lo, hi) / denom * sbp.MMPS_TO_KNOTS


def closure_m(
    xy: list[tuple[float, float]],
    start: int,
    end: int,
    variant: Variant,
    distance_m: float,
) -> float | None:
    if distance_m >= sbp.ALPHA_FULL_DISTANCE_TOLERANCE_START_M and variant.near_full_closure_offsets:
        offsets = variant.near_full_closure_offsets
    else:
        offsets = variant.closure_offsets or (
            (variant.closure_start_offset, variant.closure_end_offset),
        )
    best: float | None = None
    for start_offset, end_offset in offsets:
        a = start + start_offset
        b = end + end_offset
        if a < 0 or b < 0 or a >= len(xy) or b >= len(xy):
            continue
        x0, y0 = xy[a]
        x1, y1 = xy[b]
        closure = ((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5
        if best is None or closure < best:
            best = closure
    return best


def alpha_candidates(
    rows: list[dict[str, float]],
    radius_m: int,
    variant: Variant,
    ignore_first_start: bool = False,
) -> list[Candidate]:
    xy = sbp.project_xy(rows)
    speed_prefix = prefix(row_speed_values(rows, xy, variant))
    filtered_prefix = prefix(1.0 if row.get("filtered", False) else 0.0 for row in rows)
    position_prefix = build_segment_prefix(rows, xy)
    out: list[Candidate] = []
    min_distance_m = max(
        variant.min_distance_m,
        radius_m * variant.min_distance_radius_factor,
    )

    min_start = 1 if ignore_first_start else 0
    for end in range(len(rows)):
        for start in range(end, min_start - 1, -1):
            elapsed = elapsed_s(start, end, variant)
            if elapsed <= 0:
                continue
            if elapsed > sbp.ALPHA_MAX_TIME_S:
                break
            distance_m = candidate_distance_m(speed_prefix, position_prefix, start, end, variant)
            if distance_m > sbp.ALPHA_MAX_M:
                break
            if distance_m < min_distance_m:
                continue
            closure = closure_m(xy, start, end, variant, distance_m)
            if closure is None:
                continue
            radius_limit = (
                radius_m + variant.full_tolerance_m
                if distance_m >= sbp.ALPHA_FULL_DISTANCE_TOLERANCE_START_M
                else radius_m + variant.short_tolerance_m
            )
            if closure > radius_limit:
                continue
            speed_kn = candidate_speed_kn(speed_prefix, start, end, variant)
            if speed_kn <= 0.0:
                continue
            x0, y0 = xy[start]
            max_radius_m = 0.0
            for point_index in range(start, end + 1):
                x1, y1 = xy[point_index]
                radius = ((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5
                if radius > max_radius_m:
                    max_radius_m = radius
            out.append(
                Candidate(
                    speed_kn,
                    int(rows[start]["row"]),
                    int(rows[end]["row"]),
                    distance_m,
                    closure,
                    end - start + 1,
                    elapsed,
                    int(range_sum(filtered_prefix, start, end)),
                    max_radius_m,
                )
            )
    out.sort(key=lambda item: item.speed_kn, reverse=True)
    return out


def select_non_overlapping(candidates: list[Candidate], count: int) -> list[Candidate]:
    selected: list[Candidate] = []
    for candidate in candidates:
        overlaps = any(
            not (candidate.end_row < used.start_row or candidate.start_row > used.end_row)
            for used in selected
        )
        if overlaps:
            continue
        selected.append(candidate)
        if len(selected) >= count:
            break
    return selected


def select_chrono_eps(candidates: list[Candidate], count: int, eps_kn: float) -> list[Candidate]:
    """Chronological overlap selector used to test Speedreader tie handling."""
    selected: list[Candidate] = []
    for candidate in sorted(candidates, key=lambda item: (item.end_row, -item.start_row)):
        overlap_indexes = [
            index
            for index, used in enumerate(selected)
            if not (candidate.end_row < used.start_row or candidate.start_row > used.end_row)
        ]
        if not overlap_indexes:
            selected.append(candidate)
            continue

        best_overlap = max(overlap_indexes, key=lambda index: selected[index].speed_kn)
        if candidate.speed_kn <= selected[best_overlap].speed_kn + eps_kn:
            continue

        selected = [
            used
            for index, used in enumerate(selected)
            if index not in overlap_indexes
        ]
        selected.append(candidate)

    selected.sort(key=lambda item: item.speed_kn, reverse=True)
    return selected[:count]


def select_candidates(
    candidates: list[Candidate],
    count: int,
    selector: str,
    eps_kn: float,
) -> list[Candidate]:
    if selector == "allow-overlap":
        return candidates[:count]
    if selector == "fastest":
        return select_non_overlapping(candidates, count)
    if selector == "chrono-eps":
        return select_chrono_eps(candidates, count, eps_kn)
    if selector.startswith("by-end-"):
        return select_by_end_representative(
            candidates,
            count,
            selector.removeprefix("by-end-"),
        )
    if selector.startswith("cluster-"):
        return select_cluster_representative(candidates, count, eps_kn, selector.removeprefix("cluster-"))
    raise ValueError(f"unknown selector {selector}")


def select_by_end_representative(candidates: list[Candidate], count: int, mode: str) -> list[Candidate]:
    by_end: dict[int, list[Candidate]] = {}
    for candidate in candidates:
        by_end.setdefault(candidate.end_row, []).append(candidate)

    representatives: list[Candidate] = []
    for group in by_end.values():
        if mode == "fastest":
            representatives.append(max(group, key=lambda item: item.speed_kn))
        elif mode == "shortest":
            representatives.append(min(group, key=lambda item: item.samples))
        elif mode == "longest":
            representatives.append(max(group, key=lambda item: item.samples))
        elif mode == "lowest-closure":
            representatives.append(min(group, key=lambda item: item.closure_m))
        else:
            raise ValueError(f"unknown end representative mode {mode}")
    representatives.sort(key=lambda item: item.speed_kn, reverse=True)
    return select_non_overlapping(representatives, count)


def direct_overlap_cluster(seed: Candidate, candidates: list[Candidate]) -> list[Candidate]:
    return [
        candidate
        for candidate in candidates
        if not (candidate.end_row < seed.start_row or candidate.start_row > seed.end_row)
    ]


def select_cluster_representative(
    candidates: list[Candidate],
    count: int,
    tolerance_fraction: float,
    mode: str,
) -> list[Candidate]:
    remaining = list(candidates)
    selected: list[Candidate] = []
    while remaining and len(selected) < count:
        seed = remaining[0]
        cluster = direct_overlap_cluster(seed, remaining)
        threshold = seed.speed_kn * (1.0 - tolerance_fraction)
        near_best = [candidate for candidate in cluster if candidate.speed_kn >= threshold]
        if mode == "min-maxr":
            chosen = min(near_best, key=lambda item: (item.max_radius_m, -item.speed_kn))
        elif mode == "min-maxr-then-latest":
            chosen = min(near_best, key=lambda item: (item.max_radius_m, -item.end_row, -item.speed_kn))
        elif mode == "latest":
            chosen = max(near_best, key=lambda item: (item.end_row, item.speed_kn))
        elif mode == "shortest":
            chosen = min(near_best, key=lambda item: (item.samples, -item.speed_kn))
        elif mode == "safest-closure":
            chosen = min(near_best, key=lambda item: (item.closure_m, -item.speed_kn))
        elif mode == "max-closure":
            chosen = max(near_best, key=lambda item: (item.closure_m, item.speed_kn))
        elif mode == "fast-high-max-closure":
            if seed.speed_kn >= 15.0:
                chosen = seed
            else:
                chosen = max(near_best, key=lambda item: (item.closure_m, item.speed_kn))
        else:
            raise ValueError(f"unknown cluster representative mode {mode}")
        selected.append(chosen)
        remaining = [
            candidate
            for candidate in remaining
            if candidate.end_row < chosen.start_row or candidate.start_row > chosen.end_row
        ]
    selected.sort(key=lambda item: item.speed_kn, reverse=True)
    return selected[:count]


def score_selected(selected: list[Candidate], truth: tuple[Truth, ...]) -> tuple[float, int, int]:
    score = 0.0
    speed_hits = 0
    window_hits = 0
    for truth_item in truth:
        if truth_item.rank > len(selected):
            continue
        candidate = selected[truth_item.rank - 1]
        speed_delta = abs(candidate.speed_kn - truth_item.speed_kn)
        if speed_delta <= 0.005:
            score += 2.0
            speed_hits += 1
        elif speed_delta <= 0.020:
            score += 1.0
        if (
            truth_item.start_row is not None
            and truth_item.end_row is not None
            and candidate.start_row == truth_item.start_row
            and candidate.end_row == truth_item.end_row
        ):
            score += 3.0
            window_hits += 1
    return score, speed_hits, window_hits


def raw_rank(candidates: list[Candidate], truth: Truth) -> int | None:
    if truth.start_row is None or truth.end_row is None:
        return None
    for index, candidate in enumerate(candidates, start=1):
        if candidate.start_row == truth.start_row and candidate.end_row == truth.end_row:
            return index
    return None


def speed_matches(candidates: list[Candidate], speed_kn: float, tolerance_kn: float = 0.005) -> list[tuple[int, Candidate]]:
    return [
        (index, candidate)
        for index, candidate in enumerate(candidates, start=1)
        if abs(candidate.speed_kn - speed_kn) <= tolerance_kn
    ]


def time_to_seconds(text: str) -> float:
    hours, minutes, seconds = text.split(":")
    return int(hours) * 3600.0 + int(minutes) * 60.0 + float(seconds)


def row_time_seconds(row: dict[str, float]) -> float:
    value = row["time"].time()
    return (
        value.hour * 3600.0
        + value.minute * 60.0
        + value.second
        + value.microsecond / 1_000_000.0
    )


def circular_seconds_delta(a: float, b: float) -> float:
    delta = abs(a - b) % 86400.0
    return min(delta, 86400.0 - delta)


def resolve_truth_end_row(rows: list[dict[str, float]], case: Case, truth: Truth) -> int | None:
    if truth.end_row is not None:
        return truth.end_row
    if truth.end_time is None:
        return None

    target = (time_to_seconds(truth.end_time) - case.speedreader_utc_offset_hours * 3600.0) % 86400.0
    best = min(rows, key=lambda row: circular_seconds_delta(row_time_seconds(row), target))
    return int(best["row"])


def find_truth_candidate(
    candidates: list[Candidate],
    rows: list[dict[str, float]],
    case: Case,
    truth: Truth,
    tolerance_kn: float = 0.010,
) -> tuple[Candidate | None, int | None]:
    end_row = resolve_truth_end_row(rows, case, truth)
    if truth.start_row is not None and end_row is not None:
        for candidate in candidates:
            if candidate.start_row == truth.start_row and candidate.end_row == end_row:
                return candidate, end_row
    if end_row is None:
        return None, None

    matches = [
        candidate
        for candidate in candidates
        if candidate.end_row == end_row
        and abs(candidate.speed_kn - truth.speed_kn) <= tolerance_kn
    ]
    if matches:
        return min(matches, key=lambda item: abs(item.speed_kn - truth.speed_kn)), end_row
    return None, end_row


def nearest_at_end_row(
    candidates: list[Candidate],
    end_row: int,
    speed_kn: float,
    count: int = 3,
) -> list[Candidate]:
    same_end = [candidate for candidate in candidates if candidate.end_row == end_row]
    same_end.sort(key=lambda item: abs(item.speed_kn - speed_kn))
    return same_end[:count]


def compare_variant(
    cases: tuple[Case, ...],
    radii: tuple[int, ...],
    variant: Variant,
    selector: str,
    eps_kn: float,
    totals_only: bool = False,
) -> None:
    total_score = 0.0
    total_speed_hits = 0
    total_window_hits = 0
    if not totals_only:
        print(f"\nVARIANT {variant.name} selector={selector} eps={eps_kn:.3f}")
    for case in cases:
        rows = load_case(case)
        if not case.truth:
            continue
        if not totals_only:
            print(f"\n{case.name}")
        for radius in radii:
            if radius not in case.truth:
                continue
            candidates = alpha_candidates(rows, radius, variant, case.start_row > 1)
            selected = select_candidates(candidates, 5, selector, eps_kn)
            score, speed_hits, window_hits = score_selected(selected, case.truth[radius])
            total_score += score
            total_speed_hits += speed_hits
            total_window_hits += window_hits
            if totals_only:
                continue
            print(f"  alpha {radius}m score={score:.1f} speed_hits={speed_hits}/5 windows={window_hits}")
            for index, candidate in enumerate(selected, start=1):
                target = case.truth[radius][index - 1] if index <= len(case.truth[radius]) else None
                target_text = ""
                if target:
                    target_text = f" target={target.speed_kn:.3f}"
                    target_end = resolve_truth_end_row(rows, case, target)
                    if target_end is not None:
                        target_text += f" target_end={target_end}"
                    if target.start_row is not None and target_end is not None:
                        target_for_rank = Truth(target.rank, target.speed_kn, target.start_row, target_end)
                        rank = raw_rank(candidates, target_for_rank)
                        target_text += f" target_window={target.start_row}->{target_end} raw_rank={rank}"
                print(
                    f"    #{index}: {candidate.speed_kn:7.3f} "
                    f"{candidate.start_row}->{candidate.end_row} "
                    f"dist={candidate.distance_m:.1f} close={candidate.closure_m:.1f}"
                    f"{target_text}"
                )
    prefix = "TOTAL" if totals_only else f"\nTOTAL"
    print(
        f"{prefix} {variant.name} selector={selector} eps={eps_kn:.3f}: "
        f"score={total_score:.1f} speed_hits={total_speed_hits} "
        f"window_hits={total_window_hits}"
    )


def print_target_clusters(
    cases: tuple[Case, ...],
    radius: int,
    variant: Variant,
    selector: str,
    eps_kn: float,
) -> None:
    print(
        f"\nTARGET CLUSTERS variant={variant.name} "
        f"selector={selector} eps={eps_kn:.3f} alpha={radius}m"
    )
    for case in cases:
        if not case.truth or radius not in case.truth:
            continue
        rows = load_case(case)
        row_times = {
            int(row["row"]): row["time"].time()
            for row in rows
        }
        candidates = alpha_candidates(rows, radius, variant, case.start_row > 1)
        selected = select_candidates(candidates, 5, selector, eps_kn)
        selected_by_window = {(item.start_row, item.end_row): index for index, item in enumerate(selected, start=1)}
        print(f"\n{case.name}")
        for truth_item in case.truth[radius]:
            target, resolved_end_row = find_truth_candidate(candidates, rows, case, truth_item)
            end_text = (
                f" end_time={truth_item.end_time} resolved_end={resolved_end_row}"
                if truth_item.end_time or resolved_end_row is not None
                else ""
            )

            if target is None:
                matches = speed_matches(candidates, truth_item.speed_kn)
                print(
                    f"  Speedreader #{truth_item.rank}: {truth_item.speed_kn:.3f} "
                    f"speed_matches={len(matches)}{end_text}"
                )
                if resolved_end_row is not None:
                    nearest_end = nearest_at_end_row(
                        candidates,
                        resolved_end_row,
                        truth_item.speed_kn,
                    )
                    if nearest_end:
                        print("    nearest candidates at resolved end row:")
                        for candidate in nearest_end:
                            selected_rank = selected_by_window.get((candidate.start_row, candidate.end_row))
                            marker = f" selected=#{selected_rank}" if selected_rank else ""
                            print(
                                f"      {candidate.speed_kn:.3f} "
                                f"{candidate.start_row}->{candidate.end_row} "
                                f"dist={candidate.distance_m:.1f} close={candidate.closure_m:.1f}"
                                f"{marker}"
                            )
                for raw_index, candidate in matches[:20]:
                    selected_rank = selected_by_window.get((candidate.start_row, candidate.end_row))
                    marker = f" selected=#{selected_rank}" if selected_rank else ""
                    print(
                        f"    raw_rank={raw_index}: {candidate.speed_kn:.3f} "
                        f"{candidate.start_row}->{candidate.end_row} "
                        f"time={row_times.get(candidate.end_row)} "
                        f"dist={candidate.distance_m:.1f} close={candidate.closure_m:.1f}"
                        f"{marker}"
                    )
                if not matches:
                    nearest = sorted(
                        enumerate(candidates, start=1),
                        key=lambda item: abs(item[1].speed_kn - truth_item.speed_kn),
                    )[:3]
                    print("    nearest raw candidates:")
                    for raw_index, candidate in nearest:
                        print(
                            f"      raw_rank={raw_index}: {candidate.speed_kn:.3f} "
                            f"{candidate.start_row}->{candidate.end_row} "
                            f"time={row_times.get(candidate.end_row)} "
                            f"dist={candidate.distance_m:.1f} close={candidate.closure_m:.1f}"
                        )
                continue

            target_rank = next(
                (
                    index
                    for index, candidate in enumerate(candidates, start=1)
                    if candidate.start_row == target.start_row
                    and candidate.end_row == target.end_row
                ),
                None,
            )
            print(
                f"  Speedreader #{truth_item.rank}: {truth_item.speed_kn:.3f} "
                f"{target.start_row}->{target.end_row} raw_rank={target_rank}{end_text}"
            )

            print(
                f"    target: {target.speed_kn:.3f} dist={target.distance_m:.1f} "
                f"close={target.closure_m:.1f} samples={target.samples} filtered={target.filtered}"
            )
            blockers = [
                candidate
                for candidate in candidates
                if candidate.speed_kn > target.speed_kn
                and not (candidate.end_row < target.start_row or candidate.start_row > target.end_row)
            ][:8]
            if blockers:
                print("    faster overlapping candidates:")
                for blocker in blockers:
                    selected_rank = selected_by_window.get((blocker.start_row, blocker.end_row))
                    marker = f" selected=#{selected_rank}" if selected_rank else ""
                    print(
                        f"      {blocker.speed_kn:.3f} {blocker.start_row}->{blocker.end_row} "
                        f"time={row_times.get(blocker.end_row)} "
                        f"dist={blocker.distance_m:.1f} close={blocker.closure_m:.1f}"
                        f"{marker}"
                    )
            else:
                print("    no faster overlapping candidates")


def print_blocker_summary(
    cases: tuple[Case, ...],
    radii: tuple[int, ...],
    variant: Variant,
) -> None:
    print(f"\nBLOCKER SUMMARY variant={variant.name}")
    for case in cases:
        if not case.truth:
            continue
        rows = load_case(case)
        print(f"\n{case.name}")
        for radius in radii:
            if radius not in case.truth:
                continue
            candidates = alpha_candidates(rows, radius, variant, case.start_row > 1)
            selected = select_non_overlapping(candidates, 5)
            print(f"  alpha {radius}m")
            for truth_item in case.truth[radius]:
                target, resolved_end_row = find_truth_candidate(
                    candidates,
                    rows,
                    case,
                    truth_item,
                    tolerance_kn=0.020,
                )
                if target is None:
                    print(
                        f"    #{truth_item.rank}: target {truth_item.speed_kn:.3f} "
                        f"not found end={resolved_end_row}"
                    )
                    continue

                selected_rank = next(
                    (
                        index
                        for index, candidate in enumerate(selected, start=1)
                        if candidate.start_row == target.start_row
                        and candidate.end_row == target.end_row
                    ),
                    None,
                )
                blockers = [
                    candidate
                    for candidate in candidates
                    if candidate.speed_kn > target.speed_kn
                    and not (
                        candidate.end_row < target.start_row
                        or candidate.start_row > target.end_row
                    )
                ]
                if blockers:
                    blocker = blockers[0]
                    print(
                        f"    #{truth_item.rank}: T {target.speed_kn:6.3f} "
                        f"{target.start_row}->{target.end_row} "
                        f"d={target.distance_m:5.1f} c={target.closure_m:4.1f} "
                        f"rank={selected_rank or '-'} | "
                        f"B {blocker.speed_kn:6.3f} {blocker.start_row}->{blocker.end_row} "
                        f"d={blocker.distance_m:5.1f} c={blocker.closure_m:4.1f} "
                        f"ds={blocker.start_row - target.start_row:+4} "
                        f"de={blocker.end_row - target.end_row:+4}"
                    )
                else:
                    print(
                        f"    #{truth_item.rank}: T {target.speed_kn:6.3f} "
                        f"{target.start_row}->{target.end_row} "
                        f"d={target.distance_m:5.1f} c={target.closure_m:4.1f} "
                        f"rank={selected_rank or '-'} | no faster overlap"
                    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--variant", choices=[variant.name for variant in VARIANTS], default="speedreader-prev-start")
    parser.add_argument("--all-variants", action="store_true")
    parser.add_argument("--radius", type=int, choices=(50, 60, 70), action="append")
    parser.add_argument("--case", choices=[case.name for case in CASES], action="append")
    parser.add_argument(
        "--selector",
        choices=(
            "fastest",
            "chrono-eps",
            "allow-overlap",
            "by-end-fastest",
            "by-end-shortest",
            "by-end-longest",
            "by-end-lowest-closure",
            "cluster-latest",
            "cluster-shortest",
            "cluster-safest-closure",
            "cluster-min-maxr",
            "cluster-min-maxr-then-latest",
            "cluster-max-closure",
            "cluster-fast-high-max-closure",
        ),
        default="fastest",
    )
    parser.add_argument("--eps", type=float, default=0.003)
    parser.add_argument("--clusters", action="store_true")
    parser.add_argument("--blockers", action="store_true")
    parser.add_argument("--totals-only", action="store_true")
    args = parser.parse_args()

    selected_cases = tuple(case for case in CASES if not args.case or case.name in args.case)
    radii = tuple(args.radius) if args.radius else (50,)
    variants = VARIANTS if args.all_variants else tuple(
        variant for variant in VARIANTS if variant.name == args.variant
    )
    for variant in variants:
        if args.blockers:
            print_blocker_summary(selected_cases, radii, variant)
        elif args.clusters:
            for radius in radii:
                print_target_clusters(selected_cases, radius, variant, args.selector, args.eps)
        else:
            compare_variant(
                selected_cases,
                radii,
                variant,
                args.selector,
                args.eps,
                args.totals_only,
            )
    return 0


if __name__ == "__main__":
    sys.exit(main())
