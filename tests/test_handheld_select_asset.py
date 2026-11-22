#!/usr/bin/env python3
"""Host regression for the private walkthrough handheld-card converter."""
from pathlib import Path
import json
import sys
import tempfile

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.build_handheld_select_asset import convert, lz77_decode


def make_source(path: Path) -> None:
    image = Image.new("RGB", (640, 360))
    draw = ImageDraw.Draw(image)
    for y in range(360):
        for x in range(640):
            image.putpixel((x, y), ((x * 7) % 256, (y * 5) % 256, ((x + y) * 3) % 256))
    draw.rectangle((40, 30, 600, 330), fill=(32, 96, 48))
    draw.rectangle((80, 70, 560, 290), fill=(240, 192, 96))
    draw.text((180, 150), "FUZZ FOREST", fill=(24, 16, 64))
    image.save(path)


with tempfile.TemporaryDirectory() as temp:
    base = Path(temp)
    source = base / "source.png"
    make_source(source)
    for colors, expected_bpp, expected_bytes in ((16, 4, 16200), (256, 8, 32400)):
        prefix = base / "converted" / "ui" / f"test_{colors}"
        manifest = convert(source, prefix, f"test_{colors}", 630, colors)
        compressed = prefix.with_suffix(".bin").read_bytes()
        raw = lz77_decode(compressed)
        assert compressed[0] == 0x10
        assert len(raw) == expected_bytes
        assert manifest["pixel_bpp"] == expected_bpp
        assert manifest["packed_content_bytes"] == expected_bytes
        assert manifest["crop"] == "none"
        assert manifest["letterbox_rows"] == {"top": 12, "bottom": 13}
        assert len(manifest["palette_bgr555"]) == colors
        assert all(int(value, 16) <= 0x7FFF for value in manifest["palette_bgr555"])
        preview = Image.open(prefix.with_suffix(".preview.png")).convert("P")
        assert preview.size == (240, 160)
        assert all(preview.getpixel((x, y)) == 0
                   for y in list(range(12)) + list(range(147, 160))
                   for x in range(240))
        assert (prefix.parent.parent / f"test_{colors}_palette.c").exists()
        assert (prefix.parent.parent / f"test_{colors}_palette.h").exists()
        saved = json.loads(prefix.with_suffix(".manifest.json").read_text())
        assert saved["source_sha256"] == manifest["source_sha256"]

print("Walkthrough handheld-card asset: PASS (16/8bpp, BIOS LZ77, BGR555 palette, no crop, 12/13px letterbox)")
