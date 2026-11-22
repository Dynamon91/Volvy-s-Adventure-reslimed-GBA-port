/* Minimal, independent GBA register definitions for an EXPERIMENTAL Clang build.
 * This is NOT libgba. The normal project still builds with devkitARM/libgba. */
#ifndef VOLVY_EXPERIMENTAL_GBA_H
#define VOLVY_EXPERIMENTAL_GBA_H
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef volatile uint16_t vu16;
typedef struct { u16 x, y; } BGOffset;
typedef struct { u16 attr0, attr1, attr2, dummy; } OBJATTR;
#define ALIGN(n) __attribute__((aligned(n)))
#define VRAM ((void*)0x06000000u)
#define BG_COLORS ((volatile u16*)0x05000000u)
#define OBJ_COLORS ((volatile u16*)0x05000200u)
#define OAM ((OBJATTR*)0x07000000u)
#define REG_DISPCNT (*(vu16*)0x04000000u)
#define REG_VCOUNT (*(vu16*)0x04000006u)
#define BGCTRL ((vu16*)0x04000008u)
#define BG_OFFSET ((volatile BGOffset*)0x04000010u)
#define REG_KEYINPUT (*(vu16*)0x04000130u)
#define REG_IME (*(vu16*)0x04000208u)
#define IRQ_VBLANK 1
#define ATTR0_DISABLED (2u << 8)
#define ATTR0_SQUARE 0
#define ATTR1_SIZE_16 (1u << 14)
#define ATTR1_SIZE_32 (2u << 14)
#define OBJ_PALETTE(n) ((n) << 12)
#define MODE_0 0
#define BG0_ON (1u << 8)
#define BG1_ON (1u << 9)
#define BG2_ON (1u << 10)
#define BG3_ON (1u << 11)
#define OBJ_ON (1u << 12)
#define OBJ_1D_MAP (1u << 6)
#define BG_SIZE_0 0
#define BG_SIZE_1 (1u << 14)
#define CHAR_BASE(n) ((n) << 2)
#define SCREEN_BASE(n) ((n) << 8)
#define KEY_A (1u << 0)
#define KEY_B (1u << 1)
#define KEY_SELECT (1u << 2)
#define KEY_START (1u << 3)
#define KEY_RIGHT (1u << 4)
#define KEY_LEFT (1u << 5)
#define KEY_UP (1u << 6)
#define KEY_DOWN (1u << 7)
#define KEY_R (1u << 9)

/* BIOS-independent VBlank polling. No IRQs are enabled in this shim. */
static inline void irqInit(void) { }
static inline void irqEnable(int irq) { (void)irq; }
static inline void VBlankIntrWait(void)
{
    while (REG_VCOUNT >= 160u) { }
    while (REG_VCOUNT < 160u) { }
}
static inline void SetMode(u16 mode) { REG_DISPCNT = mode; }

/* GBA DMA3 copies 32-bit units; all original project callsites are 4B aligned. */
#define REG_DMA3_SRC (*(volatile u32*)0x040000D4u)
#define REG_DMA3_DST (*(volatile u32*)0x040000D8u)
#define REG_DMA3_CNT (*(volatile u32*)0x040000DCu)
static inline void dmaCopy(const void *src, void *dst, u32 size)
{
    if (!size) return;
    while (REG_DMA3_CNT & (1u << 31)) { }
    REG_DMA3_SRC = (u32)(uintptr_t)src;
    REG_DMA3_DST = (u32)(uintptr_t)dst;
    REG_DMA3_CNT = (1u << 31) | (1u << 26) | (size >> 2);
}
#endif
