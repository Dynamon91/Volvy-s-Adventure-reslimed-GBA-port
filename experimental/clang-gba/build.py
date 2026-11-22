#!/usr/bin/env python3
"""EXPERIMENTAL bare-metal GBA cartridge link using Clang/LLD, NOT libgba.

The build validates architecture, headers, RAM/ROM image and unresolved symbols.
It cannot establish that the resulting image boots or renders in mGBA/hardware.
"""
from __future__ import annotations
import os, pathlib, shutil, subprocess, sys
root = pathlib.Path(__file__).resolve().parents[2]
toolroot = pathlib.Path(__file__).resolve().parent
outdir = root / 'build-clang-experimental'
outdir.mkdir(exist_ok=True)
clang=shutil.which(os.environ.get('CLANG','clang'))
ld=shutil.which(os.environ.get('LLD','ld.lld'))
objcopy=shutil.which(os.environ.get('OBJCOPY','llvm-objcopy'))
if not all((clang,ld,objcopy)):
    sys.exit('Missing clang, ld.lld or llvm-objcopy.')
include=[toolroot/'include', root/'include',root/'assets/converted',
         root/'assets/converted/rooms',root/'assets/converted/tilesets',root/'tests/support/arm_includes']
flags=['--target=armv4t-none-eabi','-mcpu=arm7tdmi','-mthumb','-std=gnu11',
       '-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-pic',
       '-O2','-Wall','-Wextra','-Werror', '-ffunction-sections','-fdata-sections']
flags += [f'-I{p}' for p in include]
sources=[*sorted((root/'source').glob('*.c')),
         *sorted((root/'source/core').glob('*.c')),
         *sorted((root/'source/platform').glob('*.c')),
         *sorted((root/'assets/converted').glob('*.c')),
         *sorted((root/'assets/converted/rooms').glob('*.c')),
         *sorted((root/'assets/converted/tilesets').glob('*.c')),
         toolroot/'minicrt.c']
objects=[]
for i,src in enumerate(sources):
    dst=outdir/f'{i:03d}_{src.stem}.o'
    subprocess.run([clang,*flags,'-c',str(src),'-o',str(dst)],check=True)
    objects.append(dst)
crt=outdir/'crt0.o'
subprocess.run([clang,'--target=armv4t-none-eabi','-mcpu=arm7tdmi',
                '-c',str(toolroot/'crt0.s'),'-o',str(crt)],check=True)
objects.insert(0,crt)
divmod=outdir/'divmod.o'
subprocess.run([clang,'--target=armv4t-none-eabi','-mcpu=arm7tdmi',
                '-c',str(toolroot/'divmod.s'),'-o',str(divmod)],check=True)
objects.append(divmod)
elf=outdir/'volvy_reslimed.elf'
subprocess.run([ld,'-flavor','gnu','-T',str(toolroot/'gba_cart.ld'),
                '--gc-sections', '-Map',str(outdir/'volvy_reslimed.map'),
                '-o',str(elf),*[str(x) for x in objects]],check=True)
gba=outdir/'volvy_reslimed.gba'
subprocess.run([objcopy,'-O','binary',str(elf),str(gba)],check=True)
rom=bytearray(gba.read_bytes())
if len(rom)<0xC0: raise ValueError('ROM shorter than mandatory GBA header')
if rom[0xB2]!=0x96: raise ValueError('Invalid GBA fixed header byte')
rom[0xBD]=(-0x19-sum(rom[0xA0:0xBD]))&0xff
if (sum(rom[0xA0:0xBD])+rom[0xBD]+0x19)&255:
    raise ValueError('Header complement failed')
payload_size=len(rom)
# Cartridge images commonly use power-of-two ROM capacities; fill unallocated
# bytes with 0xFF so a physical flashcart has an unambiguous mask boundary.
capacity=1<<(payload_size-1).bit_length()
if capacity>32*1024*1024: raise ValueError('GBA cartridge size exceeded')
rom.extend(b'\xff'*(capacity-payload_size))
gba.write_bytes(rom)
print(f'Experimental Clang GBA image: {gba} ({len(rom):,} bytes; '+
      f'{payload_size:,} bytes payload); header check PASS; {len(sources)} C files compiled')
print('IMPORTANT: boot/rendering NOT verified; this is NOT a devkitARM/libgba build.')
