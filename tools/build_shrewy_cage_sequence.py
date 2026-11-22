#!/usr/bin/env python3
"""Bake the source seq_cage_drop Bezier curve into 60 fixed-point GBA samples."""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path


def cubic(a, b, c, d, t):
    u = 1.0 - t
    return u*u*u*a + 3*u*u*t*b + 3*u*t*t*c + t*t*t*d


def eval_curve(points, x):
    if x <= points[0]["x"]:
        return points[0]["value"]
    if x >= points[-1]["x"]:
        return points[-1]["value"]
    for a, b in zip(points, points[1:]):
        if a["x"] <= x <= b["x"]:
            ah, bh = a["bezier"], b["bezier"]
            p0x, p0y = a["x"], a["value"]
            p1x, p1y = p0x + ah[2], p0y + ah[3]
            p2x, p2y = b["x"] + bh[0], b["value"] + bh[1]
            p3x, p3y = b["x"], b["value"]
            lo, hi = 0.0, 1.0
            # Monotonic time handles in this source curve; bisection is stable.
            for _ in range(40):
                mid = (lo + hi) * 0.5
                mx = cubic(p0x, p1x, p2x, p3x, mid)
                if mx < x:
                    lo = mid
                else:
                    hi = mid
            t = (lo + hi) * 0.5
            return cubic(p0y, p1y, p2y, p3y, t)
    raise AssertionError("curve segment not found")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("sequence_json", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    doc = json.loads(args.sequence_json.read_text(encoding="utf-8"))
    seq = next((s for s in doc["sequences"] if s["name"] == "seq_cage_drop"), None)
    if seq is None:
        raise ValueError("seq_cage_drop not found")
    if seq["playback_speed"] != 60.0 or seq["length_frames"] != 60.0:
        raise ValueError("Unexpected seq_cage_drop playback metadata")
    cage_track = next((t for t in seq["tracks"] if t["type"] == "instance" and t["name"] == "obj_cage"), None)
    if cage_track is None:
        raise ValueError("obj_cage instance track missing")
    pos_track = next((t for t in cage_track["children"] if t["type"] == "real" and t["name"] == "position"), None)
    if pos_track is None or len(pos_track["keyframes"]) != 1:
        raise ValueError("Expected one position keyframe")
    key = pos_track["keyframes"][0]
    y_channel = next((c for c in key["channels"] if c["id"] == 1), None)
    if y_channel is None or not y_channel["embedded_curve"]:
        raise ValueError("Y position embedded curve missing")
    y_curve = next((c for c in y_channel["embedded_curve"]["channels"] if c["name"] == "y"), None)
    if y_curve is None or y_curve["curve_type"] != 2:
        raise ValueError("Expected a Bezier Y curve")
    points = y_curve["points"]
    key_length = key["length"]
    frames = int(seq["length_frames"])
    values = [eval_curve(points, min(1.0, frame / key_length)) for frame in range(frames)]
    q12 = [int(round(v * 4096.0)) for v in values]

    out = args.out
    out.mkdir(parents=True, exist_ok=True)
    header = f'''#ifndef VOLVY_SHREWY_CAGE_SEQUENCE_H
#define VOLVY_SHREWY_CAGE_SEQUENCE_H
#include <stdint.h>
#include "core/fixed.h"
#define SHREWY_CAGE_SEQUENCE_FPS 60
#define SHREWY_CAGE_SEQUENCE_FRAMES {frames}
#define SHREWY_CAGE_POSITION_KEY_LENGTH {int(key_length)}
extern const fx32 shrewy_cage_drop_y_q12[SHREWY_CAGE_SEQUENCE_FRAMES];
#endif
'''
    source = '''#include "shrewy_cage_sequence.h"

/* Baked from seq_cage_drop / obj_cage / position.y (source Bezier curve). */
const fx32 shrewy_cage_drop_y_q12[SHREWY_CAGE_SEQUENCE_FRAMES] = {\n'''
    for i in range(0, len(q12), 6):
        source += "    " + ", ".join(str(v) for v in q12[i:i+6]) + ",\n"
    source += "};\n"
    (out / "shrewy_cage_sequence.h").write_text(header, encoding="utf-8")
    (out / "shrewy_cage_sequence.c").write_text(source, encoding="utf-8")
    manifest = {
        "sequence": seq["name"], "playback_speed_fps": seq["playback_speed"],
        "sequence_length_frames": frames, "position_key_length_frames": key_length,
        "track": "obj_cage.position.y", "curve_type": "bezier",
        "samples_source_pixels": [round(v, 6) for v in values],
        "samples_q12": q12,
        "interpretation_note": "These are raw sequence position-channel samples. Their room-origin offset and trigger event are not decoded; do not add them to the room instance pivot without verifying the original callsite.",
    }
    (out / "shrewy_cage_sequence_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        "sequence": seq["name"], "fps": seq["playback_speed"],
        "frames": frames, "key_length": key_length,
        "start_y": round(values[0], 3), "frame20_y": round(values[20], 3),
        "frame59_y": round(values[-1], 3),
        "output": str(out),
    }, indent=2))


if __name__ == "__main__":
    main()
