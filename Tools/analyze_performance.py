"""Summarize complete UE CSV captures without hiding interior hitches.

Percentiles use nearest rank. One boundary row at each end is discarded;
all interior rows (including stalls) remain. GPU pass times can overlap.
"""
import argparse
import csv
import json
import math
import statistics
from pathlib import Path


def percentile(values, fraction):
    return sorted(values)[max(0, math.ceil(len(values) * fraction) - 1)]


def timing(values):
    return {"mean_ms": statistics.mean(values), "p50_ms": percentile(values, .5),
            "p95_ms": percentile(values, .95), "p99_ms": percentile(values, .99),
            "max_ms": max(values)}


def analyze(directory):
    directory = Path(directory)
    metadata = json.loads((directory / "Metadata.json").read_text(encoding="utf-8-sig"))
    host = json.loads((directory / "Host.json").read_text(encoding="utf-8-sig"))
    with (directory / "Capture.csv").open(encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.reader(stream))
    if len(rows) < 6 or rows[-1][:2] != ["[HasHeaderRowAtEnd]", "1"]:
        raise ValueError("CSV lacks its completed footer/header")
    header = rows[-2]
    if rows[0] != header[:len(rows[0])]:
        raise ValueError("CSV header changed incompatibly")
    data = rows[1:-2]
    if any(len(row) > len(header) or len(row) < len(rows[0]) for row in data):
        raise ValueError("Malformed CSV data row")
    # UE appends newly encountered stats to the final header. Earlier rows may
    # lack those trailing columns; they are zero until first encountered.
    data = data[1:-1]
    if len(data) < 20:
        raise ValueError("Too few measured frames")

    def column(name):
        if header.count(name) != 1:
            raise ValueError(f"Missing or ambiguous stat: {name}")
        index = header.index(name)
        values = [float(row[index]) if index < len(row) else 0. for row in data]
        if not all(math.isfinite(value) and value >= 0 for value in values):
            raise ValueError(f"Invalid stat: {name}")
        return values

    frames = column("FrameTime")
    if min(frames) <= 0:
        raise ValueError("Interior frame has no duration")
    elapsed = sum(frames) / 1000
    requested = metadata["requested_capture_seconds"]
    if not .9 * requested <= elapsed <= 1.1 * requested:
        raise ValueError(f"Capture duration {elapsed:.2f}s does not match {requested}s")
    depth = column("DivePerf/DepthM")
    chunks = column("DivePerf/TerrainChunks")
    if min(chunks) < 1 or max(chunks) > 9:
        raise ValueError("Terrain streaming escaped its expected bounds")
    scenario = metadata["scenario"]
    if scenario == "perf-underwater" and min(depth) < .5:
        raise ValueError("Underwater benchmark was not submerged")
    if scenario == "perf-shore" and not metadata["walking"]:
        raise ValueError("Shore benchmark was not standing on land")
    if scenario == "perf-surface" and statistics.median(depth) > .2:
        raise ValueError("Surface benchmark remained submerged")
    distance = column("DivePerf/DistanceM")
    if scenario == "perf-stream" and distance[-1] - distance[0] < requested * .5:
        raise ValueError("Streaming benchmark did not travel far enough")
    gpu = column("GPUTime")
    if statistics.median(gpu) <= 0:
        raise ValueError("GPU timings were not recorded")
    slowest = sorted(frames, reverse=True)[:max(1, math.ceil(len(frames) * .01))]
    gpu_passes = [(statistics.mean(column(name)), name) for name in header if name.startswith("GPU/")]
    memory = column("DivePerf/ProcessMemoryMB")
    return {
        "metadata": metadata, "host": host,
        "frames": len(frames), "discarded_boundary_frames": 2,
        "measured_seconds": elapsed, "average_fps": len(frames) / elapsed,
        "one_percent_low_fps": 1000 / statistics.mean(slowest),
        "frame": timing(frames), "game_thread": timing(column("GameThreadTime")),
        "render_thread": timing(column("RenderThreadTime")), "rhi_thread": timing(column("RHIThreadTime")),
        "gpu": timing(gpu),
        "frames_over_16_67_ms": sum(v > 1000 / 60 for v in frames),
        "frames_over_33_33_ms": sum(v > 1000 / 30 for v in frames),
        "frames_over_100_ms": sum(v > 100 for v in frames),
        "all_frames_within_30fps_budget": max(frames) <= 1000 / 30,
        "memory_mb": {"start": memory[0], "end": memory[-1], "max": max(memory)},
        "terrain_chunks": {"min": min(chunks), "max": max(chunks),
                           "frames_below_nine": sum(value < 9 for value in chunks)},
        "distance_m": distance[-1] - distance[0],
        "top_gpu_passes_ms_not_additive": [{"name": name, "mean_ms": value}
            for value, name in sorted(gpu_passes, reverse=True)[:12]],
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    report = analyze(args.directory)
    (args.directory / "Summary.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    f = report["frame"]
    print(f"PERF_REPORT fps={report['average_fps']:.2f} low1={report['one_percent_low_fps']:.2f} "
          f"p95={f['p95_ms']:.2f}ms p99={f['p99_ms']:.2f}ms max={f['max_ms']:.2f}ms "
          f"over33ms={report['frames_over_33_33_ms']}/{report['frames']}")


if __name__ == "__main__":
    main()
