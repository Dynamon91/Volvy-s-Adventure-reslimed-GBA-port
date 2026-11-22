
#ifndef FAKE_GBA_H
#define FAKE_GBA_H
#include <stdint.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef volatile uint16_t vu16;
typedef struct { u16 x, y; } BGOffset;
typedef struct { u16 attr0, attr1, attr2, dummy; } OBJATTR;
#define ALIGN(n) __attribute__((aligned(n)))
#define VRAM ((void*)0x06000000)
#define BG_COLORS ((u16*)0x05000000)
#define OBJ_COLORS ((u16*)0x05000200)
#define OAM ((OBJATTR*)0x07000000)
#define REG_KEYINPUT (*(vu16*)0x04000130)
#define REG_IME (*(vu16*)0x04000208)
#define IRQ_VBLANK 1
#define ATTR0_DISABLED (2u << 8)
#define ATTR0_SQUARE 0
#define ATTR1_SIZE_16 (1u << 14)
#define ATTR1_SIZE_32 (2u << 14)
#define OBJ_PALETTE(n) ((n) << 12)
#define MODE_0 0
#define MODE_4 4
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
static volatile u16 fake_bgctrl[4];
#define BGCTRL fake_bgctrl
static volatile BGOffset fake_bgoffset[4];
#define BG_OFFSET fake_bgoffset
static inline void irqInit(void) {}
static inline void irqEnable(int x) {(void)x;}
static inline void VBlankIntrWait(void) {}
static inline void SetMode(u16 x) {(void)x;}
static inline void dmaCopy(const void *src, void *dst, u32 size)
{ (void)src; (void)dst; (void)size; }
void LZ77UnCompWram(const void *src, void *dst);
#endif
