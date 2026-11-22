#!/usr/bin/env python3
"""Build private GBA Mode 4 assets from a 16:9 walkthrough selection-screen frame.

Preserves the full frame at 240x135 (nearest-neighbour, no crop/stretch), with
12/13px letterboxing for the 240x160 GBA display. Emits BIOS-LZ77 pixel data,
a BGR555 palette C/header, a PNG preview, and a source-hash manifest.
Original-derived image outputs belong in the private asset bundle, not public Git.
"""
from __future__ import annotations
import argparse, hashlib, json, re
from pathlib import Path
from PIL import Image

W, H, CONTENT_H, TOP = 240, 160, 135, 12


def bgr555(rgb):
    r, g, b = rgb
    return ((r * 31 + 127) // 255) | (((g * 31 + 127) // 255) << 5) | (((b * 31 + 127) // 255) << 10)


def ident(value):
    value = re.sub(r"\W+", "_", value).strip("_").lower()
    return value if value and not value[0].isdigit() else "asset_" + value


def pack4(indices):
    if len(indices) & 1:
        raise ValueError("4bpp pixel count must be even")
    return bytes(indices[i] | (indices[i + 1] << 4) for i in range(0, len(indices), 2))


def lz77_encode(src):
    if len(src) > 0xFFFFFF:
        raise ValueError("BIOS LZ77 output exceeds 24-bit size")
    out = bytearray((0x10, len(src) & 255, (len(src) >> 8) & 255, (len(src) >> 16) & 255))
    chains = {}
    pos = 0
    while pos < len(src):
        flag_at = len(out)
        out.append(0)
        flags = 0
        for bit in range(8):
            if pos >= len(src):
                break
            best_len = best_dist = 0
            if pos + 2 < len(src):
                key = src[pos:pos + 3]
                candidates = chains.get(key, [])
                while candidates and pos - candidates[0] > 4096:
                    candidates.pop(0)
                for candidate in reversed(candidates[-96:]):
                    dist = pos - candidate
                    length = 3
                    limit = min(18, len(src) - pos)
                    while length < limit and src[candidate + length] == src[pos + length]:
                        length += 1
                    if length > best_len:
                        best_len, best_dist = length, dist
                        if length == 18:
                            break
            if best_len >= 3:
                flags |= 1 << (7 - bit)
                disp = best_dist - 1
                out.extend((((best_len - 3) << 4) | ((disp >> 8) & 15), disp & 255))
                consumed = best_len
            else:
                out.append(src[pos])
                consumed = 1
            for i in range(pos, min(pos + consumed, len(src) - 2)):
                chains.setdefault(src[i:i + 3], []).append(i)
            pos += consumed
        out[flag_at] = flags
    while len(out) & 3:
        out.append(0)
    return bytes(out)


def lz77_decode(blob):
    if len(blob) < 4 or blob[0] != 0x10:
        raise ValueError("bad BIOS LZ77 header")
    size = blob[1] | (blob[2] << 8) | (blob[3] << 16)
    out = bytearray()
    pos = 4
    while len(out) < size:
        flags = blob[pos]
        pos += 1
        for bit in range(8):
            if len(out) >= size:
                break
            if flags & (1 << (7 - bit)):
                a, b = blob[pos], blob[pos + 1]
                pos += 2
                length = (a >> 4) + 3
                dist = (((a & 15) << 8) | b) + 1
                if dist > len(out):
                    raise ValueError("invalid BIOS LZ77 distance")
                for _ in range(min(length, size - len(out))):
                    out.append(out[-dist])
            else:
                out.append(blob[pos])
                pos += 1
    return bytes(out)


def convert(source: Path, prefix: Path, name: str, source_time=None, colors=256):
    source_bytes = source.read_bytes()
    image = Image.open(source).convert("RGB")
    if image.width * 9 != image.height * 16:
        raise ValueError(f"Expected 16:9 input, got {image.width}x{image.height}")
    if colors not in (16, 256):
        raise ValueError("colors must be 16 or 256")
    content = image.resize((W, CONTENT_H), Image.Resampling.NEAREST)
    quant = content.quantize(colors=colors, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    indices = list(quant.getdata())
    rawpal = quant.getpalette() or []
    palette = [tuple(rawpal[i:i + 3]) for i in range(0, colors * 3, 3)]
    palette += [(0, 0, 0)] * (colors - len(palette))
    bg = image.getpixel((0, 0))
    bg_index = min(range(colors), key=lambda i: sum((palette[i][c] - bg[c]) ** 2 for c in range(3)))
    order = [bg_index] + [i for i in range(colors) if i != bg_index]
    remap = {old: new for new, old in enumerate(order)}
    indices = [remap[i] for i in indices]
    palette = [palette[i] for i in order]
    packed = pack4(indices) if colors == 16 else bytes(indices)
    expected = W * CONTENT_H // (2 if colors == 16 else 1)
    if len(packed) != expected:
        raise AssertionError("packed content size mismatch")
    compressed = lz77_encode(packed)
    if lz77_decode(compressed) != packed:
        raise AssertionError("LZ77 round-trip failed")

    name = ident(name)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    prefix.with_suffix(".bin").write_bytes(compressed)
    palette_dir = prefix.parent.parent
    palette_dir.mkdir(parents=True, exist_ok=True)
    hp = palette_dir / f"{name}_palette.h"
    cp = palette_dir / f"{name}_palette.c"
    guard = f"VOLVY_{name.upper()}_PALETTE_H"
    hp.write_text(f"#ifndef {guard}\n#define {guard}\n#include <stdint.h>\nextern const uint16_t g_{name}_palette[{colors}];\n#endif\n")
    words = [bgr555(c) for c in palette]
    cp.write_text(f'#include "{hp.name}"\nconst uint16_t g_{name}_palette[{colors}] __attribute__((aligned(4))) = {{\n    ' + ", ".join(f"0x{x:04X}" for x in words) + "\n};\n")

    frame = [0] * (W * H)
    frame[TOP * W:(TOP + CONTENT_H) * W] = indices
    preview = Image.new("P", (W, H))
    preview.putdata(frame)
    flat = [v for rgb in palette for v in rgb]
    preview.putpalette(flat + [0] * (768 - len(flat)))
    preview.save(prefix.with_suffix(".preview.png"))
    preview.resize((W * 3, H * 3), Image.Resampling.NEAREST).save(
        prefix.with_name(prefix.name + "_preview_3x.png"))

    manifest = {
        "asset": name, "source_file": source.name,
        "source_sha256": hashlib.sha256(source_bytes).hexdigest(),
        "source_time_seconds": source_time, "source_size": list(image.size),
        "target_size": [W, H], "content_size": [W, CONTENT_H],
        "crop": "none", "resampling": "nearest-neighbour",
        "letterbox_rows": {"top": TOP, "bottom": H - TOP - CONTENT_H},
        "pixel_bpp": 4 if colors == 16 else 8,
        "palette_entries": colors, "packed_content_bytes": len(packed),
        "lz77_bytes": len(compressed), "palette_bgr555": [f"0x{x:04X}" for x in words],
        "letterbox_palette_index": 0, "palette_header": hp.name, "palette_source": cp.name,
        "runtime_integration": "not integrated; private asset pipeline only"
    }
    prefix.with_suffix(".manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input", type=Path)
    ap.add_argument("--name", required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--source-time", type=int)
    ap.add_argument("--colors", type=int, choices=(16, 256), default=256)
    a = ap.parse_args()
    print(json.dumps(convert(a.input, a.out, a.name, a.source_time, a.colors), indent=2))


if __name__ == "__main__":
    main()
