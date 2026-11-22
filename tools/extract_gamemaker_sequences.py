#!/usr/bin/env python3
"""Extract GameMaker SEQN resources and keyframe curves to JSON.

Parser follows the public GameMaker sequence resource layout: SEQN version,
sequence pointer table, nested tracks, keyframes, channels and embedded curves.
The default version gate matches the supplied Volvy archive (GM 2026.0.0.23).
"""
from __future__ import annotations
import argparse
import json
import math
import re
import struct
import sys
from pathlib import Path

TRACK_TYPES = {
    "GMGroupTrack": "group",
    "GMGraphicTrack": "graphic",
    "GMInstanceTrack": "instance",
    "GMSequenceTrack": "sequence",
    "GMAudioTrack": "audio",
    "GMTextTrack": "text",
    "GMRealTrack": "real",
    "GMColourTrack": "colour",
    "GMBoolTrack": "bool",
    "GMStringTrack": "string",
    "GMSpriteFramesTrack": "sprite_frames",
}
TYPE_REAL = 6
TYPE_COLOUR = 7
TYPE_TEXT = 5
TYPE_AUDIO = 4
TYPE_STRING = 9
TYPE_MESSAGE = 11
TYPE_MOMENT = 12


class Reader:
    def __init__(self, data: bytes, game):
        self.data = data
        self.game = game
        self.pos = 0

    def seek(self, pos: int):
        if pos < 0 or pos > len(self.data):
            raise ValueError(f"SEQN seek outside file: 0x{pos:X}")
        self.pos = pos

    def align4(self):
        self.pos = (self.pos + 3) & ~3

    def _take(self, n: int) -> bytes:
        if self.pos + n > len(self.data):
            raise ValueError(f"SEQN truncated at 0x{self.pos:X}, need {n} bytes")
        b = self.data[self.pos:self.pos+n]
        self.pos += n
        return b

    def u32(self): return struct.unpack("<I", self._take(4))[0]
    def i32(self): return struct.unpack("<i", self._take(4))[0]
    def f32(self): return struct.unpack("<f", self._take(4))[0]
    def ptrstr(self):
        ptr = self.u32()
        return self.game.read_name(ptr) if ptr else ""


def safe_float(v):
    if not math.isfinite(v):
        return None
    return round(float(v), 7)


def read_anim_curve(r: Reader, named: bool, version):
    name = r.ptrstr() if named else ""
    graph_type = r.u32()
    channel_count = r.u32()
    if channel_count > 64:
        raise ValueError("unreasonable animation-curve channel count")
    channels = []
    for _ in range(channel_count):
        channel_name = r.ptrstr()
        curve_type = r.u32()
        iterations = r.u32()
        point_count = r.u32()
        if point_count > 4096:
            raise ValueError("unreasonable animation-curve point count")
        points = []
        for _ in range(point_count):
            x, value = r.f32(), r.f32()
            if version >= (2, 3, 1, 0):
                handles = [r.f32(), r.f32(), r.f32(), r.f32()]
            else:
                handles = [None, None, None, None]
                r._take(4)
            points.append({
                "x": safe_float(x), "value": safe_float(value),
                "bezier": [safe_float(v) for v in handles],
            })
        channels.append({
            "name": channel_name, "curve_type": curve_type,
            "iterations": iterations, "points": points,
        })
    return {"name": name, "graph_type": graph_type, "channels": channels}


def read_keys(r: Reader, track_type: int, version):
    r.align4()
    count = r.u32()
    if count > 10000:
        raise ValueError("unreasonable sequence keyframe count")
    keys = []
    for _ in range(count):
        key = r.f32()
        length = r.f32()
        stretch = bool(r.u32())
        disabled = bool(r.u32())
        channel_count = r.u32()
        if channel_count > 64:
            raise ValueError("unreasonable sequence key channel count")
        channels = []
        for _ in range(channel_count):
            channel_id = r.i32()
            channel = {"id": channel_id}
            if track_type in (TYPE_REAL, TYPE_COLOUR):
                channel["value"] = safe_float(r.f32()) if track_type == TYPE_REAL else r.i32()
                embedded = bool(r.u32())
                channel["curve_index"] = r.i32()
                channel["embedded_curve"] = read_anim_curve(r, False, version) if embedded else None
            elif track_type in (TYPE_MESSAGE, TYPE_MOMENT):
                message_count = r.u32()
                if message_count > 1024:
                    raise ValueError("unreasonable sequence message count")
                channel["messages"] = [r.ptrstr() for _ in range(message_count)]
            elif track_type == TYPE_TEXT:
                channel["text"] = r.ptrstr()
                channel["wrap"] = bool(r.u32())
                channel["alignment"] = r.i32()
                channel["font_index"] = r.i32()
                if version >= (2024, 14, 0, 0):
                    r._take(8)
            elif track_type == TYPE_STRING:
                channel["text"] = r.ptrstr()
            else:
                channel["integer"] = r.i32()
                if track_type == TYPE_AUDIO:
                    r._take(4)
                    channel["audio_mode"] = r.i32()
            channels.append(channel)
        keys.append({
            "frame": safe_float(key), "length": safe_float(length),
            "stretch": stretch, "disabled": disabled, "channels": channels,
        })
    return keys


def read_track(r: Reader, version, depth=0):
    if depth > 12:
        raise ValueError("sequence track nesting too deep")
    model = r.ptrstr()
    if model not in TRACK_TYPES:
        raise ValueError(f"unsupported sequence track type {model!r} at 0x{r.pos-4:X}")
    name = r.ptrstr()
    builtin_name = r.u32()
    traits = r.u32()
    creation_track = bool(r.u32())
    tag_count = r.u32()
    curve_count = r.u32()
    child_count = r.u32()
    if tag_count > 1024 or curve_count > 1024 or child_count > 1024:
        raise ValueError("unreasonable sequence track metadata count")
    tags = [r.i32() for _ in range(tag_count)]
    curves = []
    for _ in range(curve_count):
        resource_type = r.ptrstr()
        if resource_type != "GMAnimCurve":
            raise ValueError(f"unsupported sequence resource {resource_type!r}")
        curves.append(read_anim_curve(r, True, version))
    children = [read_track(r, version, depth + 1) for _ in range(child_count)]
    track_type = list(TRACK_TYPES).index(model)
    result = {
        "model": model, "type": TRACK_TYPES[model], "name": name,
        "builtin_name": builtin_name, "traits": traits,
        "creation_track": creation_track,
        "visible": 1 not in tags, "enabled": 2 not in tags,
        "tags": tags, "curves": curves, "children": children,
        "interpolation": None, "keyframes": [],
    }
    if track_type == 0:
        return result
    if track_type in (TYPE_REAL, TYPE_COLOUR):
        r.align4()
        result["interpolation"] = r.i32()
    result["keyframes"] = read_keys(r, track_type, version)
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("data_win", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--gm-version", default="2026.0.0.23")
    ap.add_argument("--only", action="append", default=[])
    args = ap.parse_args()

    version = tuple(int(x) for x in re.findall(r"\d+", args.gm_version)[:4])
    version = version + (0,) * (4 - len(version))
    sys.path.insert(0, str(Path(__file__).resolve().parent / "vendor"))
    from gm_extract.gamedata import GameData

    game = GameData(str(args.data_win))
    off, size = game.chunk("SEQN")
    r = Reader(game.data, game)
    r.seek(off)
    chunk_version = r.u32()
    if chunk_version != 1:
        raise ValueError(f"unsupported SEQN chunk version {chunk_version}")
    count = r.u32()
    if count > 4096:
        raise ValueError("unreasonable SEQN resource count")
    pointers = [r.u32() for _ in range(count)]
    sequences = []
    for index, ptr in enumerate(pointers):
        if not ptr:
            continue
        r.seek(ptr)
        name = r.ptrstr()
        playback = r.u32()
        playback_speed = r.f32()
        playback_speed_type = r.u32()
        length = r.f32()
        origin_x = r.i32()
        origin_y = r.i32()
        volume = r.f32()
        if version >= (2024, 13, 0, 0):
            r._take(8)
        messages = read_keys(r, TYPE_MESSAGE, version)
        track_count = r.u32()
        if track_count > 4096:
            raise ValueError("unreasonable sequence track count")
        tracks = [read_track(r, version) for _ in range(track_count)]
        function_count = r.u32()
        if function_count > 4096:
            raise ValueError("unreasonable sequence function count")
        functions = [{"id": r.i32(), "name": r.ptrstr()} for _ in range(function_count)]
        moments = read_keys(r, TYPE_MOMENT, version)
        sequences.append({
            "index": index, "name": name, "playback": playback,
            "playback_speed": safe_float(playback_speed),
            "playback_speed_type": playback_speed_type,
            "length_frames": safe_float(length),
            "origin": [origin_x, origin_y], "volume": safe_float(volume),
            "messages": messages, "tracks": tracks,
            "functions": functions, "moments": moments,
        })

    if args.only:
        sequences = [s for s in sequences if s["name"] in set(args.only)]
    report = {
        "source": args.data_win.name,
        "game_name": game.game_name,
        "gm_version": args.gm_version,
        "seqn_chunk_version": chunk_version,
        "sequence_count": len(pointers),
        "sequences": sequences,
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({
        "sequence_count": len(pointers),
        "sequence_names": [s["name"] for s in sequences],
        "output": str(args.out),
    }, indent=2))


if __name__ == "__main__":
    main()
