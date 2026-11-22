#!/usr/bin/env python3
"""Summarize the supplied rm_w1_b1_00 player traces against room metadata.

The traces record the player and selected interaction events, not opponent AI
state. This tool intentionally reports only measurements present in the files.
"""
from __future__ import annotations
import argparse
import collections
import csv
import json
from pathlib import Path


def read_trace(path: Path):
    columns = []
    player = []
    events = []
    room_name = None
    with path.open("r", newline="", encoding="utf-8-sig") as handle:
        for row in csv.reader(handle):
            if not row:
                continue
            if row[0] == "#room" and len(row) > 1:
                room_name = row[1]
            elif row[0] == "#cols":
                columns = row[1:]
            elif row[0] == "E":
                if len(row) >= 5:
                    events.append({
                        "t": int(row[1]), "x": float(row[2]), "y": float(row[3]),
                        "type": row[4], "arg1": row[5] if len(row) > 5 else "",
                        "arg2": row[6] if len(row) > 6 else "",
                        "arg3": row[7] if len(row) > 7 else ""
                    })
            elif columns and len(row) >= len(columns):
                item = dict(zip(columns, row))
                for key in ("t", "onions", "swole", "underground", "combo", "time_ms", "image", "face"):
                    item[key] = int(float(item[key]))
                for key in ("x", "y", "hspd", "vspd"):
                    item[key] = float(item[key])
                player.append(item)
    return room_name, player, events


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("room_json", type=Path)
    ap.add_argument("shadow_json", type=Path)
    ap.add_argument("trace_dir", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    room = json.loads(args.room_json.read_text(encoding="utf-8"))
    shadow = json.loads(args.shadow_json.read_text(encoding="utf-8"))
    if room["name"] != "rm_w1_b1_00" or shadow["room"] != room["name"]:
        raise ValueError("Expected matching rm_w1_b1_00 room and shadow metadata")

    segments = []
    all_player = []
    all_events = []
    for seg in shadow["segments"]:
        clip = next(iter(seg.get("clips", {}).values()), None)
        if not clip:
            continue
        path = args.trace_dir / Path(clip).name
        if not path.exists():
            raise FileNotFoundError(path)
        trace_room, player, events = read_trace(path)
        if trace_room != room["name"]:
            raise ValueError(f"{path.name} identifies room {trace_room}, expected {room['name']}")
        if not player:
            raise ValueError(f"No player frames in {path}")
        all_player.extend(player)
        all_events.extend(events)
        segments.append({
            "segment": seg["seg"], "trace_file": path.name,
            "shadow_anchor": [seg["x"], seg["y"]],
            "shadow_size": [seg["w"], seg["h"]],
            "shadow_timed": bool(seg["timed"]),
            "frames": len(player),
            "start": {"t": player[0]["t"], "x": player[0]["x"], "y": player[0]["y"],
                      "time_ms": player[0]["time_ms"]},
            "end": {"t": player[-1]["t"], "x": player[-1]["x"], "y": player[-1]["y"],
                    "time_ms": player[-1]["time_ms"]},
            "bounds": {
                "x": [min(p["x"] for p in player), max(p["x"] for p in player)],
                "y": [min(p["y"] for p in player), max(p["y"] for p in player)]
            },
            "states": dict(collections.Counter(p["state"] for p in player)),
            "sprites": dict(collections.Counter(p["sprite"] for p in player)),
            "events": dict(collections.Counter(e["type"] for e in events)),
            "event_count": len(events)
        })

    if not all_player:
        raise ValueError("No trace frames found")
    event_counts = collections.Counter(e["type"] for e in all_events)
    event_targets = collections.Counter(e["arg1"] for e in all_events if e["arg1"])
    instances = room["instances"]
    race_starts = [i for i in instances if i["object"] == "obj_race_start"]
    race_finishes = [i for i in instances if i["object"] == "obj_race_finish"]
    if len(race_starts) != 2 or len(race_finishes) != 1:
        raise ValueError("Unexpected race gate count in source room")

    first = all_player[0]
    start_gate = min(race_starts, key=lambda i: abs(i["y"] - first["y"]))
    start_crossing = next((p for p in all_player if p["x"] >= start_gate["x"]
                           and abs(p["y"] - start_gate["y"]) <= 48), None)
    # Keep clip-local clocks separate. The four source clips have overlapping
    # time_ms ranges, so their timestamps cannot be subtracted across segments.
    finish_events = []
    finish_player = None
    finish_segment_start_ms = None
    if segments:
        last_clip = args.trace_dir / segments[-1]["trace_file"]
        _room, last_players, last_events = read_trace(last_clip)
        finish_events = [e for e in last_events if e["type"] == "race"]
        if finish_events:
            finish_event = finish_events[-1]
            finish_player = min(last_players, key=lambda p: abs(p["t"] - finish_event["t"]))
            finish_segment_start_ms = last_players[0]["time_ms"]
    finish_event = finish_events[-1] if finish_events else None

    continuity = []
    for a, b in zip(segments, segments[1:]):
        continuity.append({
            "from_segment": a["segment"], "to_segment": b["segment"],
            "end_to_start_dx": round(b["start"]["x"] - a["end"]["x"], 4),
            "end_to_start_dy": round(b["start"]["y"] - a["end"]["y"], 4)
        })

    # This is only the elapsed time inside the final clip, not the full race.
    final_clip_elapsed = None
    if finish_player is not None and finish_segment_start_ms is not None:
        final_clip_elapsed = finish_player["time_ms"] - finish_segment_start_ms

    report = {
        "room": room["name"], "room_size_source": [room["width"], room["height"]],
        "room_instance_count": len(instances),
        "source_race_objects": {
            "race_start": len(race_starts), "race_finish": len(race_finishes),
            "race_podium": sum(i["object"] == "obj_race_podium" for i in instances),
            "race_crowd": sum(i["object"] == "obj_race_crowd" for i in instances),
            "bunny": sum(i["object"] == "obj_bunny" for i in instances),
            "bunny_ground": sum(i["object"] == "obj_bunny_ground" for i in instances)
        },
        "segments": segments,
        "segment_continuity": continuity,
        "total_player_frames": len(all_player),
        "total_event_records": len(all_events),
        "event_types": dict(event_counts),
        "event_targets": dict(event_targets),
        "player_bounds_source": {
            "x": [min(p["x"] for p in all_player), max(p["x"] for p in all_player)],
            "y": [min(p["y"] for p in all_player), max(p["y"] for p in all_player)]
        },
        "player_state_counts": dict(collections.Counter(p["state"] for p in all_player)),
        "player_sprite_counts": dict(collections.Counter(p["sprite"] for p in all_player)),
        "swole_frames": sum(p["swole"] != 0 for p in all_player),
        "underground_frames": sum(p["underground"] != 0 for p in all_player),
        "race_timing_trace_estimate": {
            "start_gate_source_position": [start_gate["x"], start_gate["y"]],
            "start_gate_crossing": ({"t": start_crossing["t"], "x": start_crossing["x"],
                                      "y": start_crossing["y"], "time_ms": start_crossing["time_ms"]}
                                     if start_crossing else None),
            "finish_event": finish_event,
            "finish_player_time_ms_in_final_clip": finish_player["time_ms"] if finish_player else None,
            "final_clip_start_time_ms": finish_segment_start_ms,
            "final_clip_start_to_finish_event_ms": final_clip_elapsed,
            "gate_to_finish_elapsed_ms": None,
            "qualification": "Full race time is not calculable: the four clip time_ms ranges overlap and are not a shared continuous clock. The final-clip interval is not the whole race."
        },
        "limits": [
            "The traces record the player and selected interaction events, not bunny opponent positions or AI state.",
            "The race elapsed time is estimated from crossing the source start-gate x coordinate to the recorded race event.",
            "No Swolvy frames occur in these four selected segment traces."
        ]
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    sample_path = args.out.with_name("bonus_w1_trace_frames.csv")
    fields = ["segment", "t", "x", "y", "hspd", "vspd", "state", "inputs",
              "onions", "swole", "underground", "combo", "time_ms", "sprite", "image", "face"]
    with sample_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for seg in segments:
            _room, players, _events = read_trace(args.trace_dir / seg["trace_file"])
            for p in players:
                writer.writerow({"segment": seg["segment"], **{k: p[k] for k in fields if k != "segment"}})

    print(json.dumps({
        "room": room["name"], "segments": len(segments),
        "player_frames": len(all_player), "event_records": len(all_events),
        "event_types": dict(event_counts),
        "final_clip_start_to_finish_event_ms": final_clip_elapsed,
        "summary": str(args.out), "frames_csv": str(sample_path)
    }, indent=2))


if __name__ == "__main__":
    main()
