#ifndef VOLVY_TEST_STAGE_CLEAR_FAKE_GBA_H
#define VOLVY_TEST_STAGE_CLEAR_FAKE_GBA_H
#include <stdint.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#define ALIGN(n) __attribute__((aligned(n)))
#define MODE_0 0u
#define MODE_4 4u
#define BG0_ON (1u << 8)
#define BG2_ON (1u << 10)
#define OBJ_ON (1u << 12)
#define OBJ_1D_MAP (1u << 6)
extern u16 test_bg_colors[256];
extern u8 test_vram[240u * 160u];
extern u16 test_mode;
#define BG_COLORS test_bg_colors
#define STAGE_CLEAR_FRAMEBUFFER ((void *)test_vram)
static inline void SetMode(u16 mode) { test_mode = mode; }
static inline void dmaCopy(const void *src, void *dst, u32 size)
{
    memcpy(dst, src, size);
}
void LZ77UnCompWram(const void *source, void *destination);
#endif
