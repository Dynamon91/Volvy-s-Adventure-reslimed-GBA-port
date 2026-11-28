#!/usr/bin/env python3
"""Validate all-world preview reference names and hardware-palette geometry.

No private game data, ffmpeg, emulator, or ROM is required for these checks.
"""
import copy
import json
from pathlib import Path
import sys
import tempfile

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_world_reference_previews as preview

reference = ROOT / "progress/reference_scene_checkpoints.json"
data = preview.load_index(reference)
checkpoints = data["checkpoints"]
assert len(checkpoints) == 12
assert checkpoints[0]["id"] == "fudge_forest"
assert checkpoints[-1]["id"] == "late_boss"
assert checkpoints[4]["label"] == "Rigatoni Rooftops"
assert checkpoints[7]["label"] == "Macaroni Metro"
assert all(not c["course_in_native_gba"] for c in checkpoints)
assert [c["second"] for c in checkpoints] == sorted(
    c["second"] for c in checkpoints
)

source = Image.new("RGB", (640, 360), (254, 55, 7))
for y in range(120, 235):
    for x in range(190, 390):
        source.putpixel((x, y), (11, 171, 221))
output = preview.make_mode4_preview(source)
assert output.size == (240, 160)
assert output.mode == "P"
for y in tuple(range(12)) + tuple(range(147, 160)):
    assert all(output.getpixel((x, y)) == 0 for x in range(240))
for y in range(12, 147):
    assert all(1 <= output.getpixel((x, y)) <= 255 for x in range(240))

# The PNG palette is exactly the re-expanded GBA BGR555 representation.
palette = output.getpalette()
for i in range(256):
    rgb = tuple(palette[i * 3:i * 3 + 3])
    assert rgb == preview.bgr555_preview_rgb(preview.color_to_bgr555(rgb))
assert output.convert("RGB").getpixel((0, 0)) == (0, 0, 0)

try:
    preview.make_mode4_preview(Image.new("RGB", (320, 240)))
except ValueError:
    pass
else:
    raise AssertionError("Non-16:9 frames were not rejected")

with tempfile.TemporaryDirectory() as work:
    folder = Path(work)
    bad = copy.deepcopy(data)
    bad["checkpoints"][1]["second"] = bad["checkpoints"][0]["second"]
    test_path = folder / "invalid_index.json"
    test_path.write_text(json.dumps(bad))
    try:
        preview.load_index(test_path)
    except ValueError:
        pass
    else:
        raise AssertionError("Duplicate source timestamp accepted")

    bad = copy.deepcopy(data)
    bad["checkpoints"][0]["course_in_native_gba"] = True
    test_path.write_text(json.dumps(bad))
    try:
        preview.load_index(test_path)
    except ValueError:
        pass
    else:
        raise AssertionError("Unverified native GBA course accepted")

print("12-world preview pipeline: PASS (video evidence, GBA 8bpp palette, 12/13 borders, no invented levels)")
