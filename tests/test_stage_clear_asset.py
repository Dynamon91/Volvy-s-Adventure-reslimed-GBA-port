#!/usr/bin/env python3
"""Validate the walkthrough-derived 16-color GBA Stage Clear bitmap asset."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
ASSET = ROOT / "assets/converted/ui/stage_clear_v198.bin"
SOURCE = ROOT / "source/platform/stage_clear_screen.c"

blob = ASSET.read_bytes()
assert len(blob) == 8780, f"unexpected compressed asset size: {len(blob)}"
assert len(blob) % 4 == 0, "BIOS LZ77 source must be word-aligned/padded"
assert blob[0] == 0x10, "missing BIOS LZ77 header"
expected_size = blob[1] | (blob[2] << 8) | (blob[3] << 16)
assert expected_size == 16200, f"unexpected packed 4bpp size: {expected_size}"

def lz77_decode(data: bytes) -> bytes:
    out = bytearray()
    pos = 4
    while len(out) < expected_size:
        flags = data[pos]
        pos += 1
        for bit in range(8):
            if len(out) >= expected_size:
                break
            if flags & (1 << (7 - bit)):
                a, b = data[pos], data[pos + 1]
                pos += 2
                length = (a >> 4) + 3
                displacement = (((a & 0x0F) << 8) | b) + 1
                assert displacement <= len(out), "invalid LZ77 back-reference"
                for _ in range(length):
                    out.append(out[-displacement])
                    if len(out) == expected_size:
                        break
            else:
                out.append(data[pos])
                pos += 1
                if len(out) == expected_size:
                    break
    return bytes(out)

packed = lz77_decode(blob)
assert len(packed) == 16200
pixels = []
for byte in packed:
    pixels.extend((byte & 0x0F, byte >> 4))
assert len(pixels) == 240 * 135
assert set(pixels) == set(range(1, 16)), "expected all 15 image colors plus reserved index 0"

frame = [[0] * 240 for _ in range(160)]
for y in range(135):
    frame[y + 12] = pixels[y * 240:(y + 1) * 240]
assert all(not any(row) for row in frame[:12])
assert all(not any(row) for row in frame[147:])
assert any(any(row) for row in frame[12:147])

source = SOURCE.read_text()
match = re.search(r"s_stage_clear_palette\[16\]\s*=\s*\{([^}]*)\}", source, re.S)
assert match, "missing 16-color BGR555 palette"
palette = [int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{4})", match.group(1))]
assert len(palette) == 16 and palette[0] == 0x488B
assert all(v <= 0x7FFF for v in palette)
print("Stage Clear asset: PASS (BIOS LZ77, 240x135 4bpp, 16-color palette, 12/13px letterbox)")
