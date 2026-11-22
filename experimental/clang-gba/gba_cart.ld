/* Minimal cartridge layout, separate from the official devkitARM/libgba link. */
ENTRY(_start)
MEMORY {
  ROM (rx) : ORIGIN = 0x08000000, LENGTH = 32M
  EWRAM (rwx) : ORIGIN = 0x02000000, LENGTH = 256K
}
SECTIONS {
  .rom : ALIGN(4) {
    KEEP(*(.gba_header))
    . = ALIGN(4);
    *(.text .text.*)
    . = ALIGN(4);
    *(.rodata .rodata.*)
    . = ALIGN(4);
    *(.ARM.extab*)
  } > ROM
  .ARM.exidx : ALIGN(4) { *(.ARM.exidx*) } > ROM
  .data : ALIGN(4) {
    __data_start = .;
    *(.data .data.*)
    . = ALIGN(4);
    __data_end = .;
  } > EWRAM AT>ROM
  __data_load_start = LOADADDR(.data);
  .bss (NOLOAD) : ALIGN(4) {
    __bss_start = .;
    *(.bss .bss.* COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > EWRAM
  __ewram_end = .;
  ASSERT(__ewram_end < ORIGIN(EWRAM) + LENGTH(EWRAM), "EWRAM budget exceeded")
}
