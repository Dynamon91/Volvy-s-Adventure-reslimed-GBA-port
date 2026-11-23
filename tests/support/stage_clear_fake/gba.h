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
extern unsigned int test_dma3_in_progress;
extern unsigned int test_dma3_wait_calls;
extern unsigned int test_dma3_overlapped;
extern unsigned int test_mode_unblanked_early;
static inline void test_wait_dma3(void)
{
    ++test_dma3_wait_calls;
    test_dma3_in_progress = 0u;
}
#define VOLVY_BITMAP_WAIT_DMA3() test_wait_dma3()
extern u16 test_mode_trace[16];
extern unsigned int test_mode_trace_count;
#define BG_COLORS test_bg_colors
#define STAGE_CLEAR_FRAMEBUFFER ((void *)test_vram)
#define HANDHELD_CARD_FRAMEBUFFER ((void *)test_vram)
static inline void SetMode(u16 mode) {
    if ((mode & (1u << 7)) == 0u && test_dma3_in_progress)
        ++test_mode_unblanked_early;
    test_mode = mode;
    if (test_mode_trace_count < 16u)
        test_mode_trace[test_mode_trace_count++] = mode;
}
static inline void dmaCopy(const void *src, void *dst, u32 size)
{
    if (test_dma3_in_progress)
        ++test_dma3_overlapped;
    memcpy(dst, src, size);
    test_dma3_in_progress = 1u;
}
void LZ77UnCompWram(const void *source, void *destination);
#endif
