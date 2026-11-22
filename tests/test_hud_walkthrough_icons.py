#!/usr/bin/env python3
"""Validate the walkthrough-derived 16x16 onion and stopwatch BG icons."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / "include/platform/hud_walkthrough_icons.h").read_text()
ui = (root / "source/platform/ui.c").read_text()

def macro(name):
    match = re.search(rf"^#define\\s+{name}\\s+(\\d+)u?$", header, re.M)
    assert match, f"missing {name}"
    return int(match.group(1))

assert macro("VOLVY_HUD_ICON_TILE_BASE") == 40
assert macro("VOLVY_HUD_ICON_TILE_COUNT") == 8
assert macro("VOLVY_HUD_ICON_BYTES") == 256
assert macro("VOLVY_HUD_ONION_TILE") == 40
assert macro("VOLVY_HUD_STOPWATCH_TILE") == 44

palette_match = re.search(
    r"g_hud_icon_palette\\[16\\]\\s*=\\s*\\{([^}]*)\\}", header, re.S
)
assert palette_match, "missing 16-entry icon palette"
palette = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{4})", palette_match.group(1))]
assert len(palette) == 16
assert palette[0] == 0x0000 and palette[1] == 0x7FFF
assert all(0 <= color <= 0x7FFF for color in palette)

tiles_match = re.search(
    r"g_hud_icon_tiles\\[256\\]\\s*=\\s*\\{([^}]*)\\}", header, re.S
)
assert tiles_match, "missing 256-byte icon tile payload"
tiles = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", tiles_match.group(1))]
assert len(tiles) == 256
indices = {n for byte in tiles for n in (byte & 0x0F, byte >> 4)}
assert indices <= set(range(16))
assert 0 in indices and 1 in indices, "transparent/white HUD palette entries missing"
assert indices & set(range(2, 8)), "onion icon palette is empty"
assert indices & set(range(8, 16)), "stopwatch icon palette is empty"

assert '#include "platform/hud_walkthrough_icons.h"' in ui
assert 'dmaCopy(g_hud_icon_tiles, UI_FONT_VRAM + VOLVY_HUD_FONT_BYTES,' in ui
assert 'BG_COLORS[UI_PALETTE_BANK*16u+i] = g_hud_icon_palette[i];' in ui
assert 'draw_icon16(1, 1, VOLVY_HUD_ONION_TILE);' in ui
assert 'draw_str(3, 1, count);' in ui
assert 'draw_icon16(time_column, 1, VOLVY_HUD_STOPWATCH_TILE);' in ui
assert 'draw_str(time_column+2, 1, time_text);' in ui
assert 'time_column = 30 - (int)time_length - 2;' in ui
assert '(timer_frames / 60u) * 100u + ((timer_frames % 60u) * 100u) / 60u' in ui
print("Walkthrough HUD icon assets/layout: PASS (two 16x16 4bpp icons, palette and map bounds)")
