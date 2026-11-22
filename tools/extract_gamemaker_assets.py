#!/usr/bin/env python3
"""Extract selected GameMaker data.win sprites and BGND tilesets to PNG.

The parser subset in tools/vendor/gm_extract is derived from gm-extract
(MIT License, copyright 2026 NazarHK). Original game assets remain private.
"""

from __future__ import annotations
import argparse
import json
import re
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS / "vendor"))

from gm_extract.gamedata import GameData
from gm_extract.resources import parse_sprites, parse_backgrounds
from gm_extract.png import crop_rgba, place_on_canvas, write_png

PLAYER_SPRITES = {
    "spr_idle", "spr_walk", "spr_jump", "spr_float", "spr_dash",
    "spr_dash_up", "spr_grind", "spr_ground_pound", "spr_dig",
    "spr_punch", "spr_punch_up", "spr_swim", "spr_swim_dash",
    "spr_volvy_hurt", "spr_volvy_collect", "spr_volvy_windup",
    "spr_volvy_clear", "spr_volvy_clear_2", "spr_volvy_title",
    "bg_blue", "bg_clouds_1", "bg_clouds", "bg_hills_1", "bg_hills", "bg_shrubs",
    "bg_pink", "bg_clouds__1", "bg_trees_2", "bg_trees_1",
    "bg_shrubs_3", "bg_shrubs_2_top", "bg_shrubs_2",
    "bg_burrow_1", "bg_burrow_2", "bg_burrow",
    "spr_swolvy_idle", "spr_swolvy_walk", "spr_swolvy_jump",
    "spr_swolvy_float", "spr_swolvy_grind", "spr_swolvy_ground_pound",
    "spr_swolvy_punch", "spr_swolvy_punch_up", "spr_swolvy_swim",
    "spr_swolvy_transform", "spr_dig_swole",
    "spr_onion", "spr_destructible_block", "spr_action_block",
    "spr_bunny_hop", "spr_destructible_block_swolvy",
    "spr_bumper_sq", "spr_bumper_tri",
    "spr_squirrel_idle", "spr_squirrel_walk", "spr_squirrel_hurt",
    "spr_springboard", "spr_action_block_spent",
    "spr_moving_platform", "spr_cloud_platform", "spr_corner_bumper",
    "spr_hivefly", "spr_crowly_fly", "spr_vinechomp_base",
    "spr_spikes_dave", "spr_cannon", "spr_button_off", "spr_hole",
    "spr_rail_slice", "spr_bkg_space_onion", "spr_worm", "spr_plants",
    "spr_level_exit_hole", "spr_platform_placeholder",
    # World 1 burrow hazards and dedicated Shrewy boss encounter.
    "spr_autodig_hitbox", "spr_autohole", "spr_mega_boulder",
    "spr_mole_dig_enter", "spr_mole_dig_exit_no_inbetween",
    "spr_mole_dig_exit", "spr_mole_dig_surface",
    "spr_mole_dig_underground_hurt", "spr_mole_dig_underground_hurtmask",
    "spr_mole_dig_underground", "spr_mole_hole", "spr_mole_hurt",
    "spr_mole_idle_surface_digging", "spr_mole_idle_surface",
    "spr_hivefly", "spr_hivefly_hurt",
    "spr_snake_hurt", "spr_snake_idle", "spr_snake_walk",
    "spr_tnt", "spr_tnt_triggered",
    "spr_shrewy_dropkick", "spr_shrewy_hurt", "spr_shrewy_idle",
    "spr_shrewy_jump", "spr_shrewy_rock", "spr_shrewy_stomp",
    "spr_shrewy_tornado_slam", "spr_shrewy_tornado", "spr_shrewy_windup",
    "spr_cage_grunfus", "spr_owp", "spr_emitter_direction",
}

def safe_name(name):
    name = re.sub(r'[^A-Za-z0-9_.-]+', '_', name or "").strip("._")
    return name or "unnamed"

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("data_win", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--all-sprites", action="store_true")
    ap.add_argument("--no-tilesets", action="store_true")
    ap.add_argument("--only-sprites", nargs="+", default=None,
                    help="Extract only these named sprites; useful for focused tests")
    args = ap.parse_args()

    g = GameData(str(args.data_win))
    sprites = parse_sprites(g)
    tilesets = parse_backgrounds(g)

    out = args.out
    sprite_dir = out / "sprites"
    tileset_dir = out / "tilesets"
    sprite_dir.mkdir(parents=True, exist_ok=True)
    if not args.no_tilesets:
        tileset_dir.mkdir(parents=True, exist_ok=True)

    if args.only_sprites:
        requested = set(args.only_sprites)
        selected = [s for s in sprites if s["name"] in requested]
        missing = requested - {s["name"] for s in selected}
        if missing:
            raise ValueError(f"Unknown source sprite resources: {sorted(missing)}")
    else:
        selected = sprites if args.all_sprites else [
            s for s in sprites if s["name"] in PLAYER_SPRITES
        ]

    page_cache = {}
    manifest = {
        "game": g.game_name,
        "data_win_size": args.data_win.stat().st_size,
        "sprite_count_in_data_win": len(sprites),
        "tileset_count_in_data_win": len(tilesets),
        "selected_sprites": [],
        "tilesets": [],
        "errors": [],
        "source_tool": "gm-extract parser subset (MIT; see vendor/gm_extract/LICENSE)",
    }

    def get_page(page_id):
        if page_id not in page_cache:
            page_cache[page_id] = g.decode_page(page_id)
        return page_cache[page_id]

    for s in selected:
        name = safe_name(s["name"])
        frames = []
        for frame_index, tpag_ptr in enumerate(s["textures"]):
            t = g.tpag(tpag_ptr)
            if not t:
                manifest["errors"].append(f"{s['name']} frame {frame_index}: missing TPAG")
                continue
            src_x, src_y, src_w, src_h, dst_x, dst_y = t[:6]
            page_id = t[10]
            if page_id >= len(g.textures):
                manifest["errors"].append(f"{s['name']} frame {frame_index}: invalid page {page_id}")
                continue
            try:
                page_w, page_h, rgba = get_page(page_id)
                cropped, w, h = crop_rgba(rgba, page_w, page_h,
                                          src_x, src_y, src_w, src_h)
                padded, out_w, out_h = place_on_canvas(
                    cropped, w, h, s["width"], s["height"], dst_x, dst_y
                )
                frame_path = sprite_dir / name / f"frame_{frame_index:03d}.png"
                frame_path.parent.mkdir(parents=True, exist_ok=True)
                write_png(str(frame_path), out_w, out_h, padded)
                frames.append({
                    "frame": frame_index, "file": str(frame_path.relative_to(out)),
                    "page": page_id, "source_rect": [src_x, src_y, src_w, src_h],
                    "destination": [dst_x, dst_y],
                })
            except Exception as exc:
                manifest["errors"].append(f"{s['name']} frame {frame_index}: {exc}")

        if frames:
            manifest["selected_sprites"].append({
                "index": s["index"], "name": s["name"], "width": s["width"], "height": s["height"],
                "origin": [s["origin_x"], s["origin_y"]],
                "frame_count": len(frames), "frames": frames,
            })

    if not args.no_tilesets:
        for ts in tilesets:
            t = g.tpag(ts["tex_ptr"])
            if not t:
                continue
            src_x, src_y, src_w, src_h = t[:4]
            page_id = t[10]
            if page_id >= len(g.textures):
                continue
            try:
                page_w, page_h, rgba = get_page(page_id)
                cropped, w, h = crop_rgba(rgba, page_w, page_h,
                                          src_x, src_y, src_w, src_h)
                file_path = tileset_dir / f"{safe_name(ts['name'])}.png"
                write_png(str(file_path), w, h, cropped)
                manifest["tilesets"].append({
                    "index": ts["index"], "name": ts["name"], "file": str(file_path.relative_to(out)),
                    "image_size": [w, h], "tile_size": [ts["tile_w"], ts["tile_h"]],
                    "columns": ts["columns"], "tile_count": ts["tile_count"],
                    "border": [ts["border_x"], ts["border_y"]],
                })
            except Exception as exc:
                manifest["errors"].append(f"tileset {ts['name']}: {exc}")

    (out / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(f"GameMaker game: {g.game_name}")
    print(f"Sprite resources: {len(sprites)}; selected/extracted: {len(manifest['selected_sprites'])}")
    print(f"Tileset resources: {len(tilesets)}; extracted: {len(manifest['tilesets'])}")
    print(f"Extraction errors: {len(manifest['errors'])}")
    print(f"Manifest: {out / 'manifest.json'}")

if __name__ == "__main__":
    main()
