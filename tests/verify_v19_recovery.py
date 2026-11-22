#!/usr/bin/env python3
"""Validate preservation of the original v1.9 ROM and development ZIP.

This tool checks byte identity/integrity and GBA cartridge header structure.
It does NOT boot the ROM or prove real-hardware correctness.
"""
from __future__ import annotations
import argparse
import hashlib
import pathlib
import zipfile

ROM_SHA = "caab13e2988eca40aade3145643249df592ab7e5e4ab4b9d65ccde19e22ca7f2"
ZIP_SHA = "7350bcea491330a96add380ab6257d55914ac72f016bb188131dbe63030eca13"
INNER_ROM = "release/volvy_reslimed_gba_v19_experimental.gba"


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("rom", type=pathlib.Path)
    ap.add_argument("zip", type=pathlib.Path)
    args = ap.parse_args()
    rom = args.rom.read_bytes()
    archive = args.zip.read_bytes()
    assert sha256(rom) == ROM_SHA, "Wrong version or modified GBA ROM"
    assert sha256(archive) == ZIP_SHA, "ZIP differs from original user-provided v1.9"
    assert len(rom) == 2097152, "Unexpected v1.9 cartridge size"
    assert rom[0xB2] == 0x96, "Invalid GBA fixed header byte"
    assert (sum(rom[0xA0:0xBD]) + rom[0xBD] + 0x19) & 255 == 0, "GBA header complement invalid"
    with zipfile.ZipFile(args.zip) as z:
        assert z.testzip() is None, "Bad ZIP CRC"
        assert len(z.namelist()) == len(set(z.namelist())), "Duplicate ZIP entries"
        assert z.read("VERSION").strip() == b"1.9", "Wrong source version"
        assert z.read(INNER_ROM) == rom, "ROM inside ZIP differs from supplied ROM"
        assert "tests/test_invisible_marker_assets.py" in z.namelist(), "v1.9 regression missing"
        assert "previews/v19_hole_marker_before_after.png" in z.namelist(), "v1.9 preview missing"
        print(f"v1.9 ROM: PASS ({len(rom):,} B; SHA-256 {ROM_SHA})")
        print(f"v1.9 ZIP: PASS ({len(z.infolist())} entries; SHA-256 {ZIP_SHA})")
        print("Embedded ROM and uploaded ROM: BIT-FOR-BIT MATCH")
        print("Important: ROM boot and gameplay still require an mGBA/real-GBA test.")


if __name__ == "__main__":
    main()
