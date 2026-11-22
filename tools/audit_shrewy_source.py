#!/usr/bin/env python3
"""Audit source-side Shrewy room placements, script names and available traces.

This deliberately distinguishes resource-name metadata from executable GML
bytecode. It does not decompile native code or infer boss behavior.
"""
from __future__ import annotations
import argparse
import csv
import json
import sys
from collections import Counter
from pathlib import Path


def trace_summary(trace_dir: Path | None):
    if trace_dir is None:
        return {"files": [], "player_frames": 0, "boss_named_rows": 0}
    files = []
    total_frames = 0
    boss_named_rows = 0
    for path in sorted(trace_dir.glob("*.csv")):
        room_name = ""
        columns = []
        frames = 0
        boss_rows = 0
        with path.open("r", newline="", encoding="utf-8-sig") as handle:
            for row in csv.reader(handle):
                if not row:
                    continue
                if row[0] == "#room" and len(row) > 1:
                    room_name = row[1]
                elif row[0] == "#cols":
                    columns = row[1:]
                elif row[0] == "E":
                    continue
                elif columns and len(row) >= len(columns):
                    record = dict(zip(columns, row))
                    frames += 1
                    if "shrewy" in record.get("sprite", "").lower() or \
                       "shrewy" in record.get("state", "").lower():
                        boss_rows += 1
        if frames:
            total_frames += frames
            boss_named_rows += boss_rows
            files.append({
                "file": path.name,
                "room": room_name,
                "player_frames": frames,
                "shrewy_named_rows": boss_rows,
            })
    return {
        "files": files,
        "player_frames": total_frames,
        "boss_named_rows": boss_named_rows,
        "note": "CSV traces contain player-state columns; absence of Shrewy labels does not prove no boss behavior occurred outside the recorded player fields.",
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("data_win", type=Path)
    ap.add_argument("room_data", type=Path)
    ap.add_argument("--trace-dir", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    sys.path.insert(0, str(Path(__file__).resolve().parent / "vendor"))
    from gm_extract.gamedata import GameData

    game = GameData(str(args.data_win))
    scripts = game.resource_names("SCPT")
    terms = (
        "rm_shrewy", "obj_boss_shrewy", "obj_boss_shrewy_boulder",
        "obj_boulder_spawner", "obj_cage", "obj_cage_fragments",
        "obj_autohole", "obj_cone_emitter", "obj_adjustable_autodigger",
    )
    related = [
        {"index": i, "name": name}
        for i, name in enumerate(scripts)
        if any(term in name.lower() for term in terms)
    ]
    seq_off, _seq_size = game.chunk("SEQN")
    seq_version = int.from_bytes(game.data[seq_off:seq_off+4], "little")
    seq_count = int.from_bytes(game.data[seq_off+4:seq_off+8], "little")
    sequence_names = []
    for i in range(seq_count):
        ptr = int.from_bytes(game.data[seq_off+8+4*i:seq_off+12+4*i], "little")
        if ptr:
            name_ptr = int.from_bytes(game.data[ptr:ptr+4], "little")
            sequence_names.append(game.read_name(name_ptr))

    room = json.loads(args.room_data.read_text(encoding="utf-8"))
    object_counts = Counter(inst["object"] for inst in room["instances"])
    relevant_instances = [
        {
            "object": inst["object"],
            "instance_id": inst["instance_id"],
            "position": [inst["x"], inst["y"]],
            "scale": [inst["scale_x"], inst["scale_y"]],
            "rotation_degrees": inst["rotation"],
        }
        for inst in room["instances"]
        if inst["object"] in terms or inst["object"] in {
            "obj_boss_shrewy", "obj_boss_shrewy_boulder", "obj_volvy"
        }
    ]

    report = {
        "archive_resource": str(args.data_win.name),
        "game_name": game.game_name,
        "chunk_names": sorted(game.chunks),
        "script_resource_count": len(scripts),
        "code_chunk_present": game.has("CODE"),
        "sequence_chunk_version": seq_version,
        "sequence_resource_count": seq_count,
        "sequence_resource_names": sequence_names,
        "script_resource_names_related_to_shrewy": related,
        "script_source_status": (
            "SCPT resource names are present, but the data.win has no CODE chunk. "
            "This audit does not contain original GML source or a bytecode decompilation."
        ),
        "room": room["name"],
        "room_size": [room["width"], room["height"]],
        "room_instance_count": len(room["instances"]),
        "room_object_counts": dict(object_counts),
        "relevant_instance_transforms": relevant_instances,
        "trace_summary": trace_summary(args.trace_dir),
        "boss_behavior_status": (
            "Boss timings, health, vulnerability windows and hazard behavior remain provisional; "
            "the extracted room transforms are source-derived."
        ),
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({
        "game": report["game_name"],
        "script_resources": report["script_resource_count"],
        "code_chunk_present": report["code_chunk_present"],
        "sequence_resources": report["sequence_resource_names"],
        "related_script_names": len(related),
        "room_instances": report["room_instance_count"],
        "object_counts": report["room_object_counts"],
        "trace_player_frames": report["trace_summary"]["player_frames"],
        "shrewy_named_trace_rows": report["trace_summary"]["boss_named_rows"],
        "output": str(args.out),
    }, indent=2))


if __name__ == "__main__":
    main()
