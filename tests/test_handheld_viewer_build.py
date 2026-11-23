#!/usr/bin/env python3
"""Build a standalone card-viewer ARM7TDMI ROM using public synthetic assets only."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

CARDS = (
    "title_screen", "fuzz_forest", "bonus_stage_16m30", "rigatoni_rooftops",
    "selection_81m30", "bonus_stage_111m30", "fishpig", "bundtcake_burrows",
    "master_gator"
)


def zero_lz77(size: int) -> bytes:
    data = bytearray((0x10, size & 255, (size >> 8) & 255, (size >> 16) & 255))
    position = 0
    while position < size:
        flag_at = len(data)
        data.append(0)
        for bit in range(8):
            if position >= size:
                break
            if position == 0:
                data.append(0)
                position += 1
            else:
                length = min(18, size - position)
                if length < 3:
                    data.append(0)
                    position += 1
                else:
                    data[flag_at] |= 1 << (7 - bit)
                    data.extend(((length - 3) << 4, 0))
                    position += length
    while len(data) & 3:
        data.append(0)
    return bytes(data)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    shim = (root / "experimental/clang-gba/include/gba.h").read_text()
    assert "#define KEY_R (1u << 8)" in shim
    assert "#define KEY_L (1u << 9)" in shim
    payload = zero_lz77(240 * 135)
    assert payload[0] == 0x10
    assert (payload[1] | payload[2] << 8 | payload[3] << 16) == 32400
    with tempfile.TemporaryDirectory() as temp:
        folder = Path(temp)
        archive = folder / "synthetic_cards.zip"
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
            for card in CARDS:
                base = f"assets/converted/ui/{card}"
                z.writestr(base + ".bin", payload)
                z.writestr(base + ".manifest.json", json.dumps({
                    "pixel_bpp": 8, "palette_entries": 256,
                    "packed_content_bytes": 32400,
                    "source_sha256": hashlib.sha256(card.encode()).hexdigest()
                }))
                z.writestr(f"assets/converted/{card}_palette.h",
                    f"#include <stdint.h>\nextern const uint16_t g_{card}_palette[256];\n")
                z.writestr(f"assets/converted/{card}_palette.c",
                    f'#include "{card}_palette.h"\n'
                    f"const uint16_t g_{card}_palette[256] __attribute__((aligned(4))) = {{ 0x1CE7u }};\n")
        out = folder / "build"
        subprocess.run([
            sys.executable,
            str(root / "experimental/handheld-card-viewer/build.py"),
            "--assets-zip", str(archive), "--out", str(out)
        ], check=True)
        rom = (out / "volvy_handheld_cards_viewer.gba").read_bytes()
        meta = json.loads((out / "build_manifest.json").read_text())
        assert rom[0xA0:0xAC] == b"VOLVY CARDS "
        assert rom[0xAC:0xB0] == b"VLVC"
        assert rom[0xB2] == 0x96
        assert (sum(rom[0xA0:0xBE]) + 0x19) & 255 == 0
        assert len(rom) & (len(rom) - 1) == 0
        assert meta["card_count"] == len(CARDS)
        assert meta["rom_sha256"] == hashlib.sha256(rom).hexdigest()
        assert meta["mgba_boot"] == "NOT_TESTED"

        # Reject truncated streams even when their BIOS header size looks valid.
        malformed = folder / "corrupt_cards.zip"
        with zipfile.ZipFile(archive) as original, zipfile.ZipFile(malformed, "w") as bad:
            for entry in original.namelist():
                blob = original.read(entry)
                if entry == "assets/converted/ui/fuzz_forest.bin":
                    blob = blob[:4]
                bad.writestr(entry, blob)
        invalid = subprocess.run([
            sys.executable, str(root / "experimental/handheld-card-viewer/build.py"),
            "--assets-zip", str(malformed), "--out", str(folder / "bad_build")
        ], capture_output=True, text=True)
        assert invalid.returncode != 0
        assert "Truncated LZ77 flag" in invalid.stderr
    print("Standalone card viewer ARM7TDMI link / GBA checksum: PASS (synthetic assets only)")


if __name__ == "__main__":
    main()
