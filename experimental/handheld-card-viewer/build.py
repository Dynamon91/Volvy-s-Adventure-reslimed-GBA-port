#!/usr/bin/env python3
"""Build an independent reference-card GBA ROM from the user's PRIVATE video asset pack.

This is not a full game: it is a standalone Mode 4/ARM7TDMI slideshow to
validate nine converted 256-color screens. No private assets enter GitHub.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile

CARDS = (
    "title_screen", "fuzz_forest", "bonus_stage_16m30", "rigatoni_rooftops",
    "selection_81m30", "bonus_stage_111m30", "fishpig", "bundtcake_burrows",
    "master_gator"
)


def build(bundle: Path, out: Path) -> dict:
    root = Path(__file__).resolve().parents[2]
    viewer_dir = Path(__file__).resolve().parent
    experimental = root / "experimental" / "clang-gba"
    out.mkdir(parents=True, exist_ok=True)
    for name in ("clang", "ld.lld", "llvm-objcopy"):
        if not shutil.which(name):
            raise RuntimeError(f"Required tool not found: {name}")
    for directory in ("assets", "palette", "obj"):
        (out / directory).mkdir(exist_ok=True)

    declarations = []
    entries = []
    assembly = ['.section .rodata.cards,"a",%progbits', '.align 2']
    bundle_info = {}
    with zipfile.ZipFile(bundle) as zf:
        names = set(zf.namelist())
        for card in CARDS:
            prefix = f"assets/converted/ui/{card}"
            image_name = prefix + ".bin"
            manifest_name = prefix + ".manifest.json"
            pal_src = f"assets/converted/{card}_palette.c"
            pal_hdr = f"assets/converted/{card}_palette.h"
            for required in (image_name, manifest_name, pal_src, pal_hdr):
                if required not in names:
                    raise ValueError(f"Missing private asset: {required}")
            payload = zf.read(image_name)
            meta = json.loads(zf.read(manifest_name))
            if len(payload) < 4 or len(payload) % 4 or payload[0] != 0x10:
                raise ValueError(f"Invalid aligned BIOS-LZ77 blob: {image_name}")
            raw_size = payload[1] | (payload[2] << 8) | (payload[3] << 16)
            if (raw_size, meta.get("pixel_bpp"), meta.get("palette_entries")) != (32400, 8, 256):
                raise ValueError(f"Incompatible Mode 4 card: {image_name}")
            (out / "assets" / f"{card}.bin").write_bytes(payload)
            (out / "palette" / f"{card}_palette.c").write_bytes(zf.read(pal_src))
            (out / "palette" / f"{card}_palette.h").write_bytes(zf.read(pal_hdr))
            declarations += [
                f"extern const unsigned char viewer_{card}_bin[];",
                f"extern const uint16_t g_{card}_palette[256];"
            ]
            entries.append(f"    {{viewer_{card}_bin, {len(payload)}u, g_{card}_palette, 256u}},")
            assembly += [
                f".global viewer_{card}_bin", f".type viewer_{card}_bin,%object",
                f"viewer_{card}_bin:", f'.incbin "assets/{card}.bin"', ".align 2"
            ]
            bundle_info[card] = {
                "lz77_bytes": len(payload),
                "sha256": hashlib.sha256(payload).hexdigest(),
                "source_sha256": meta.get("source_sha256")
            }
    hdr = ('#ifndef VOLVY_GENERATED_VIEWER_CARDS_H\n'
           '#define VOLVY_GENERATED_VIEWER_CARDS_H\n'
           '#include <stdint.h>\n'
           '#include "platform/handheld_card_screen.h"\n')
    hdr += '\n'.join(declarations) + '\n'
    hdr += f'#define G_VIEWER_CARD_COUNT {len(CARDS)}u\n'
    hdr += 'static const HandheldCardAsset g_viewer_cards[G_VIEWER_CARD_COUNT] = {\n'
    hdr += '\n'.join(entries) + '\n};\n#endif\n'
    (out / "generated_cards.h").write_text(hdr)
    (out / "cards.s").write_text('\n'.join(assembly) + '\n')

    def invoke(args, cwd=None):
        subprocess.run(args, cwd=cwd, check=True)

    cflags = [
        "--target=armv4t-none-eabi", "-mcpu=arm7tdmi", "-mthumb",
        "-std=gnu11", "-ffreestanding", "-fno-builtin", "-fno-pic",
        "-fno-stack-protector", "-O2", "-Wall", "-Wextra", "-Werror",
        f"-I{root/'include'}", f"-I{experimental/'include'}",
        f"-I{root/'tests/support/arm_includes'}", f"-I{out}"
    ]
    sources = [
        viewer_dir / "viewer.c", root / "source/platform/handheld_card_screen.c",
        experimental / "minicrt.c"
    ] + [out / "palette" / f"{c}_palette.c" for c in CARDS]
    objects = []
    for i, src in enumerate(sources):
        obj = out / "obj" / f"{i:02d}_{src.stem}.o"
        invoke(["clang", *cflags, "-c", str(src), "-o", str(obj)])
        objects.append(obj)

    # Clone the verified original CRT header and change only its 12-byte
    # title and 4-byte game code. Keep the Nintendo logo and startup intact.
    crt = (experimental / "crt0.s").read_text()
    if '.ascii "VOLVY RESLIM"' not in crt or '.ascii "VLVY"' not in crt:
        raise ValueError("Unexpected original CRT header; refusing blind edit")
    crt = crt.replace('.ascii "VOLVY RESLIM"', '.ascii "VOLVY CARDS "')
    crt = crt.replace('.ascii "VLVY"', '.ascii "VLVC"')
    (out / "crt0_viewer.s").write_text(crt)
    for src in (out / "crt0_viewer.s", experimental / "divmod.s", out / "cards.s"):
        obj = out / "obj" / (src.stem + ".o")
        invoke(["clang", "--target=armv4t-none-eabi", "-mcpu=arm7tdmi",
                "-c", str(src), "-o", str(obj)], cwd=out)
        objects.append(obj)

    elf = out / "volvy_handheld_cards_viewer.elf"
    rom = out / "volvy_handheld_cards_viewer.gba"
    invoke(["ld.lld", "-flavor", "gnu", "-T", str(experimental / "gba_cart.ld"),
            "--gc-sections", "-Map", str(out / "volvy_handheld_cards_viewer.map"),
            "-o", str(elf), *(str(obj) for obj in objects)])
    invoke(["llvm-objcopy", "-O", "binary", str(elf), str(rom)])

    data = bytearray(rom.read_bytes())
    if len(data) < 0xC0 or data[0xB2] != 0x96 or data[0xA0:0xAC] != b"VOLVY CARDS ":
        raise ValueError("Invalid GBA header/title")
    data[0xBD] = (-0x19 - sum(data[0xA0:0xBD])) & 255
    if (sum(data[0xA0:0xBE]) + 0x19) & 255:
        raise ValueError("GBA header complement failed")
    payload_bytes = len(data)
    capacity = 1 << (len(data) - 1).bit_length()
    if capacity > 32 * 1024 * 1024:
        raise ValueError("GBA cartridge exceeds 32 MiB")
    data += b'\xff' * (capacity - len(data))
    rom.write_bytes(data)

    manifest = {
        "kind": "STANDALONE_REFERENCE_CARD_VIEWER_NOT_COMPLETE_GAME",
        "card_count": len(CARDS), "card_order": list(CARDS),
        "rom_size": len(data), "rom_payload_bytes": payload_bytes,
        "gba_header_checksum": "PASS", "armv4t_thumb_link": "PASS",
        "mgba_boot": "NOT_TESTED", "physical_hardware": "NOT_TESTED",
        "private_asset_bundle_sha256": hashlib.sha256(bundle.read_bytes()).hexdigest(),
        "rom_sha256": hashlib.sha256(data).hexdigest(),
        "assets": bundle_info
    }
    (out / "build_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({k: v for k, v in manifest.items() if k != "assets"}, indent=2))
    return manifest


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--assets-zip", required=True, type=Path)
    p.add_argument("--out", required=True, type=Path)
    args = p.parse_args()
    build(args.assets_zip, args.out.resolve())


if __name__ == "__main__":
    main()
