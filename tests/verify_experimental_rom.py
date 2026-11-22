#!/usr/bin/env python3
"""Structural checks only. Does not imply that a GBA emulator boots the ROM."""
import pathlib, re, struct, subprocess, sys
if len(sys.argv)!=3:
    sys.exit("usage: verify_experimental_rom.py image.gba image.elf")
rom=pathlib.Path(sys.argv[1]).read_bytes()
assert 0xC0 < len(rom) < 32*1024*1024
entry,=struct.unpack_from('<I',rom,0)
assert entry>>24==0xEA,"Cartridge entry must be ARM B"
branch_offset=entry&0xffffff
if branch_offset&0x800000:branch_offset-=0x1000000
assert 0x08000008+(branch_offset<<2)==0x080000C0,"Entry target must be CRT startup"
assert rom[0x04:0x08]==bytes.fromhex('24 FF AE 51')
assert rom[0xB2]==0x96
assert (0x19+sum(rom[0xA0:0xBE]))&255==0
elf=sys.argv[2]
header=subprocess.check_output(['readelf','-h',elf],text=True)
assert 'Machine:                           ARM' in header
assert 'Entry point address:               0x8000000' in header
sections=subprocess.check_output(['readelf','-S',elf],text=True)
assert '.rom' in sections and '.data' in sections and '.bss' in sections
symbols=subprocess.check_output(['readelf','-Ws',elf],text=True)
assert '__aeabi_uidivmod' in symbols and '__aeabi_uidiv' in symbols
assert ' __data_load_start' in symbols and ' __bss_end' in symbols
print(f'EXPERIMENTAL ROM STRUCTURE: PASS ({len(rom):,} B, ARM entry, logo prefix, checksum, ELF sections)')
print('EMULATOR/HARDWARE BOOT: NOT TESTED')
