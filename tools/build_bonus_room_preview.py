#!/usr/bin/env python3
"""Render source-derived rm_w1_b1_00 overview and GBA camera samples."""
from __future__ import annotations
import argparse
import csv
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


def read_player_trace(path):
    cols = []
    rows = []
    with path.open(newline="", encoding="utf-8-sig") as f:
        for row in csv.reader(f):
            if not row:
                continue
            if row[0] == "#cols":
                cols = row[1:]
            elif row[0] != "E" and not row[0].startswith("#") and cols and len(row) >= len(cols):
                item = dict(zip(cols, row))
                for key in ("t", "time_ms", "image"):
                    item[key] = int(float(item[key]))
                item["x"] = float(item["x"])
                item["y"] = float(item["y"])
                rows.append(item)
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("room_png", type=Path)
    ap.add_argument("room_json", type=Path)
    ap.add_argument("shadow_json", type=Path)
    ap.add_argument("trace_dir", type=Path)
    ap.add_argument("sprite_manifest", type=Path)
    ap.add_argument("sprite_root", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    scene = Image.open(args.room_png).convert("RGBA")
    room = json.loads(args.room_json.read_text())
    shadow = json.loads(args.shadow_json.read_text())
    sprite_manifest = json.loads(args.sprite_manifest.read_text())
    sprite_defs = {s["name"]: s for s in sprite_manifest["selected_sprites"]}
    seg_data = []
    for seg in shadow["segments"]:
        trace_name = Path(next(iter(seg["clips"].values()))).name
        rows = read_player_trace(args.trace_dir / trace_name)
        if rows:
            seg_data.append((seg, trace_name, rows))

    # Full-room map at the exact 2:1 GBA world scale.
    overview = scene.resize((scene.width // 2, scene.height // 2), Image.Resampling.NEAREST).convert("RGB")
    draw = ImageDraw.Draw(overview)
    segment_colors = [(255, 90, 80), (255, 210, 70), (80, 230, 160), (80, 190, 255)]
    for i, (seg, _name, rows) in enumerate(seg_data):
        points = [(int(round(r["x"] / 2)), int(round(r["y"] / 2))) for r in rows]
        if len(points) > 1:
            draw.line(points, fill=segment_colors[i % len(segment_colors)], width=4, joint="curve")
        sx, sy = points[0]
        ex, ey = points[-1]
        draw.ellipse((sx - 5, sy - 5, sx + 5, sy + 5), fill=segment_colors[i % len(segment_colors)], outline=(255,255,255), width=2)
        draw.ellipse((ex - 4, ey - 4, ex + 4, ey + 4), fill=(255,255,255), outline=segment_colors[i % len(segment_colors)], width=2)
        # A 240x160 GBA viewport corresponds to 480x320 source pixels.
        mid = rows[len(rows) // 2]
        left = max(0, min(overview.width - 240, int(mid["x"] / 2) - 120))
        top = max(0, min(overview.height - 160, int(mid["y"] / 2) - 80))
        draw.rectangle((left, top, left + 239, top + 159), outline=segment_colors[i % len(segment_colors)], width=3)
        draw.text((left + 4, top + 4), f"CAM {seg['seg']}", fill=(255,255,255), stroke_width=2, stroke_fill=(0,0,0))

    # Source race gates and finish area.
    starts = [i for i in room["instances"] if i["object"] == "obj_race_start"]
    finish = next(i for i in room["instances"] if i["object"] == "obj_race_finish")
    podium = next(i for i in room["instances"] if i["object"] == "obj_race_podium")
    for gate in starts:
        x, y = int(gate["x"] / 2), int(gate["y"] / 2)
        draw.rectangle((x - 8, y - 12, x + 8, y + 12), outline=(255,255,0), width=3)
    fx, fy = int(finish["x"] / 2), int(finish["y"] / 2)
    draw.rectangle((fx - 9, fy - 12, fx + 9, fy + 12), outline=(255,60,60), width=3)
    px, py = int(podium["x"] / 2), int(podium["y"] / 2)
    draw.rectangle((px - 12, py - 10, px + 12, py + 10), outline=(255,255,255), width=2)

    args.out.mkdir(parents=True, exist_ok=True)
    overview_path = args.out / "bonus_w1_race_overview_v09.png"
    overview.save(overview_path)

    # Four actual 240x160 camera crops, with the original trace sprite frame.
    panel_w, panel_h = 240, 192
    montage = Image.new("RGB", (panel_w * 2 + 28, panel_h * 2 + 56), (12, 16, 34))
    md = ImageDraw.Draw(montage)
    for i, (seg, _name, rows) in enumerate(seg_data[:4]):
        row = rows[len(rows) // 2]
        cx, cy = row["x"], row["y"]
        crop_left = max(0, min(scene.width - 480, int(round(cx - 240))))
        crop_top = max(0, min(scene.height - 320, int(round(cy - 160))))
        crop = scene.crop((crop_left, crop_top, crop_left + 480, crop_top + 320))

        sprite = None
        spr = sprite_defs.get(row["sprite"])
        if spr and spr.get("frames"):
            frame = spr["frames"][row["image"] % len(spr["frames"])]
            sprite_path = args.sprite_root / frame["file"]
            if sprite_path.exists():
                sprite = Image.open(sprite_path).convert("RGBA")
                ox, oy = spr.get("origin", [0, 0])
                crop.alpha_composite(sprite, (int(round(cx - ox - crop_left)), int(round(cy - oy - crop_top))))
        gba_crop = crop.resize((240, 160), Image.Resampling.NEAREST).convert("RGB")
        px = 8 + (i % 2) * (panel_w + 12)
        py = 44 + (i // 2) * (panel_h + 8)
        montage.paste(gba_crop, (px, py))
        label = f"SEG {seg['seg']}  t={row['t']}  {row['sprite']}"
        md.text((px, py - 16), label, fill=segment_colors[i % len(segment_colors)])
        md.rectangle((px, py, px + 239, py + 159), outline=(230, 230, 240), width=1)
    md.text((8, 6), "rm_w1_b1_00 — source trace camera samples (GBA 240x160)", fill=(255,255,255))
    sample_path = args.out / "bonus_w1_gba_camera_samples_v09.png"
    montage.save(sample_path)
    print(json.dumps({"overview": str(overview_path), "camera_samples": str(sample_path)}, indent=2))


if __name__ == "__main__":
    main()
