#ifndef VOLVY_TEST_PLAYER_SPRITES_H
#define VOLVY_TEST_PLAYER_SPRITES_H

#include <stdint.h>

/*
 * Public host tests cannot include the generated player_sprites.h because
 * source-derived sprite payloads are intentionally kept outside this repo.
 * Keep these declarations byte-for-byte compatible with the public generator
 * in tools/prepare_player_assets.py; the OBJ-part test supplies its own data.
 */
typedef struct GbaSpritePart {
    const uint8_t *tiles;
    uint16_t tile_bytes;
    uint8_t offset_x;
    uint8_t offset_y;
    uint8_t hw_w;
    uint8_t hw_h;
    uint8_t shape;
    uint8_t size_code;
} GbaSpritePart;

typedef struct GbaSpriteFrame {
    const GbaSpritePart *parts;
    uint8_t part_count;
    uint8_t width;
    uint8_t height;
    uint8_t origin_x;
    uint8_t origin_y;
} GbaSpriteFrame;

#endif
