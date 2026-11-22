#include "core/player.h"

#define PLAYER_NORMAL_W 12
#define PLAYER_NORMAL_H 10
#define PLAYER_NORMAL_ORIGIN_X 7
#define PLAYER_NORMAL_ORIGIN_Y 6
#define PLAYER_SWOLVY_W 22
#define PLAYER_SWOLVY_H 13
#define PLAYER_SWOLVY_ORIGIN_X 10
#define PLAYER_SWOLVY_ORIGIN_Y 9
#define PLAYER_NATIVE_NORMAL_W 24
#define PLAYER_NATIVE_NORMAL_H 19
#define PLAYER_NATIVE_NORMAL_ORIGIN_X 14
#define PLAYER_NATIVE_NORMAL_ORIGIN_Y 11
#define PLAYER_NATIVE_SWOLVY_W 44
#define PLAYER_NATIVE_SWOLVY_H 26
#define PLAYER_NATIVE_SWOLVY_ORIGIN_X 19
#define PLAYER_NATIVE_SWOLVY_ORIGIN_Y 17

/*
 * Source trace measurements are in original-game pixels/frame.
 * Source tiles are 16x16 and GBA tiles are 8x8: initial scale is 2:1.
 * These are measured/default-level values, not final values for every boost.
 */
#define MOVE_ACCEL       (FX_ONE * 3 / 20)       /* 0.30 / 2 */
#define MOVE_MAX_SPEED   (FX_ONE * 23 / 20)      /* 2.30 / 2 */
#define MOVE_FRICTION    (FX_ONE / 10)           /* provisional */
#define GRAVITY          (FX_ONE / 20)           /* 0.10 / 2 */
#define MAX_FALL_SPEED   (FX_ONE * 73 / 20)      /* 7.30 / 2 */
#define JUMP_SPEED       (-(FX_ONE * 69) / 40)   /* -3.45 / 2 */
#define SWOLVY_JUMP      (-(FX_ONE * 13) / 8)    /* provisional */
#define DASH_SPEED       (FX_ONE * 2)           /* 4.00 / 2 */
#define DASH_UP_SPEED    (-(FX_ONE * 2))        /* -4.00 / 2 */
#define GRIND_SPEED      (FX_ONE * 49 / 40)      /* observed median 2.45 / 2 */
#define GRIND_ACCEL      (FX_ONE / 20)           /* provisional rail control */
#define GRIND_MAX_SPEED  (FX_ONE * 7 / 2)        /* common observed peak 7 / 2 */
#define HOVER_FALL_SPEED (FX_ONE * 7 / 40)       /* 0.35 / 2 */
#define GROUND_POUND_SPEED (FX_ONE * 3)          /* 6.00 / 2 */
#define DIG_SPEED        (FX_ONE)                /* 2.00 / 2 */
#define PUNCH_UP_SPEED   (-(FX_ONE * 43) / 100)  /* -0.86 / 2 */

static int fx_floor_pixel(fx32 value)
{
    int v = fx_to_int(value);
    if (value < 0 && (value % FX_ONE)) --v;
    return v;
}

/* Grid coordinates must use mathematical floor, including pixels -1..-7.
 * C's signed integer division truncates toward zero and otherwise lets the
 * player pass 7 pixels outside a negative world boundary. */
static int tile_floor_pixel(int pixel)
{
    return pixel >= 0 ? pixel / LEVEL_TILE_SIZE :
           -1 - ((-1 - pixel) / LEVEL_TILE_SIZE);
}

static fx32 physics_scale(const Player *player, fx32 value)
{
    return player->native_scale ? value * 2 : value;
}

static void update_dimensions(Player *player)
{
    /* Preserve the source sprite pivot while the hitbox changes. */
    int pivot_x = fx_to_int(player->x) + player->origin_x;
    int pivot_y = fx_to_int(player->y) + player->origin_y;

    if (player->native_scale) {
        if (player->swole) {
            player->width = PLAYER_NATIVE_SWOLVY_W;
            player->height = PLAYER_NATIVE_SWOLVY_H;
            player->origin_x = PLAYER_NATIVE_SWOLVY_ORIGIN_X;
            player->origin_y = PLAYER_NATIVE_SWOLVY_ORIGIN_Y;
        } else {
            player->width = PLAYER_NATIVE_NORMAL_W;
            player->height = PLAYER_NATIVE_NORMAL_H;
            player->origin_x = PLAYER_NATIVE_NORMAL_ORIGIN_X;
            player->origin_y = PLAYER_NATIVE_NORMAL_ORIGIN_Y;
        }
    } else if (player->swole) {
        player->width = PLAYER_SWOLVY_W;
        player->height = PLAYER_SWOLVY_H;
        player->origin_x = PLAYER_SWOLVY_ORIGIN_X;
        player->origin_y = PLAYER_SWOLVY_ORIGIN_Y;
    } else {
        player->width = PLAYER_NORMAL_W;
        player->height = PLAYER_NORMAL_H;
        player->origin_x = PLAYER_NORMAL_ORIGIN_X;
        player->origin_y = PLAYER_NORMAL_ORIGIN_Y;
    }

    player->x = fx_from_int(pivot_x - player->origin_x);
    player->y = fx_from_int(pivot_y - player->origin_y);
}

static void collect_onions(Player *player, Level *level)
{
    int left = fx_floor_pixel(player->x);
    int top = fx_floor_pixel(player->y);
    int right = left + player->width - 1;
    int bottom = top + player->height - 1;
    int tx0 = left / LEVEL_TILE_SIZE;
    int tx1 = right / LEVEL_TILE_SIZE;
    int ty0 = top / LEVEL_TILE_SIZE;
    int ty1 = bottom / LEVEL_TILE_SIZE;
    int tx, ty;

    int gained = level_collect_world_onions(level, left, top,
                                              player->width, player->height);

    for (ty = ty0; ty <= ty1; ++ty) {
        for (tx = tx0; tx <= tx1; ++tx) {
            if (level_get_tile(level, tx, ty) == TILE_ONION) {
                level_set_tile(level, tx, ty, TILE_EMPTY);
                ++gained;
            }
        }
    }
    if (gained) {
        player->onions = (uint16_t)((player->onions + gained > 999)
                                    ? 999 : player->onions + gained);
        /* The supplied rm_w1_b1_00 trace logs 42 onions but no Swolvy
         * frames. The provisional 5-onion test transformation must not fire
         * automatically in this bonus room. The original transform trigger
         * remains unknown and should be reconstructed from source scripts. */
        if (!level->bonus_room && !player->swole &&
            player->onions >= VOLVY_ONIONS_TO_SWOLVY) {
            player->swole = 1;
            update_dimensions(player);
        }
    }
}

static int touching_hazard(const Player *player, const Level *level)
{
    int left = fx_floor_pixel(player->x);
    int top = fx_floor_pixel(player->y);
    int right = left + player->width - 1;
    int bottom = top + player->height - 1;
    int tx, ty;

    for (ty = top / 8; ty <= bottom / 8; ++ty) {
        for (tx = left / 8; tx <= right / 8; ++tx) {
            if (level_get_tile(level, tx, ty) == TILE_HAZARD) return 1;
        }
    }
    return 0;
}

/* A 32x32 source Swolvy block must not be destroyed by normal attacks. */
static int try_break_player_tile(const Player *player, Level *level, int tx, int ty)
{
    if (!player->swole && level_get_tile(level, tx, ty) == TILE_SWOLVY_BLOCK)
        return 0;
    return level_break_tile(level, tx, ty);
}

static void try_ground_dig(Player *player, Level *level)
{
    int center_x = fx_to_int(player->x) + player->width / 2;
    int foot_y = fx_to_int(player->y) + player->height + 1;
    int front_x = center_x + player->facing * (player->width / 2 + 2);
    int tx = front_x / 8;
    int ty = foot_y / 8;

    if (try_break_player_tile(player, level, tx, ty) ||
        try_break_player_tile(player, level, center_x / 8, foot_y / 8)) {
        player->hand_state = HAND_DIG;
        player->hand_timer = 15;
        player->vx = player->facing * physics_scale(player, DIG_SPEED);
        player->vy = 0;
    }
}

/* Prototype hit tests: break a nearby destructible sprite-sized tile.
 * Original enemy knockback/damage scripts are not decoded yet. */
static void try_punch_block(Player *player, Level *level, int upward)
{
    int left = fx_to_int(player->x);
    int top = fx_to_int(player->y);
    int x = left + (player->facing > 0 ? player->width + 1 : -2);
    int y = top + player->height / 2;
    int i;
    if (upward) {
        y = top - 2;
        for (i=2;i<player->width;i+=4) {
            (void)try_break_player_tile(player,level,(left+i)/8,y/8);
        }
    } else {
        for (i=2;i<player->height;i+=4) {
            (void)try_break_player_tile(player,level,x/8,(top+i)/8);
        }
    }
}

static int rail_tile_y(const Player *player, const Level *level)
{
    int center_x = fx_to_int(player->x) + player->width / 2;
    int foot_y = fx_to_int(player->y) + player->height + 2;
    int ty = foot_y / LEVEL_TILE_SIZE;

    if (level_get_tile(level, center_x / LEVEL_TILE_SIZE, ty) == TILE_RAIL) {
        return ty;
    }
    return -1;
}

/* Continuous collision against every crossed grid column/row. Checking
 * only the destination tile allows a dash, a bounce or future boosted
 * movement to tunnel through a single 8x8 barrier. Preserve the original
 * contact and destruction rules; only the tile traversal is corrected. */
static void move_horizontal(Player *player, Level *level)
{
    fx32 next_x = player->x + player->vx;
    int old_x = fx_floor_pixel(player->x);
    int new_x = fx_floor_pixel(next_x);
    int top = fx_floor_pixel(player->y);
    int bottom = top + player->height - 1;
    int y, tx;

    if (new_x > old_x) {
        int old_edge = tile_floor_pixel(old_x + player->width - 1);
        int new_edge = tile_floor_pixel(new_x + player->width - 1);
        int first_tx = new_edge > old_edge ? old_edge + 1 : new_edge;
        for (tx = first_tx; tx <= new_edge; ++tx) {
            int blocked = 0;
            for (y = top + 1; y <= bottom - 1; y += 5) {
                int ty = tile_floor_pixel(y);
                uint8_t tile = level_get_tile(level, tx, ty);
                if ((tile == TILE_BREAKABLE &&
                     (player->hand_state == HAND_DASH || player->swole)) ||
                    (tile == TILE_SWOLVY_BLOCK && player->swole)) {
                    level_break_tile(level, tx, ty);
                    continue;
                }
                if (level_is_solid(tile)) {
                    next_x = fx_from_int(tx * LEVEL_TILE_SIZE - player->width);
                    player->vx = 0;
                    if (player->hand_state == HAND_DASH) {
                        player->hand_state = HAND_NONE;
                        player->hand_timer = 0;
                    }
                    blocked = 1;
                    break;
                }
            }
            if (blocked) break;
        }
    } else if (new_x < old_x) {
        int old_edge = tile_floor_pixel(old_x);
        int new_edge = tile_floor_pixel(new_x);
        int first_tx = new_edge < old_edge ? old_edge - 1 : new_edge;
        for (tx = first_tx; tx >= new_edge; --tx) {
            int blocked = 0;
            for (y = top + 1; y <= bottom - 1; y += 5) {
                int ty = tile_floor_pixel(y);
                uint8_t tile = level_get_tile(level, tx, ty);
                if ((tile == TILE_BREAKABLE &&
                     (player->hand_state == HAND_DASH || player->swole)) ||
                    (tile == TILE_SWOLVY_BLOCK && player->swole)) {
                    level_break_tile(level, tx, ty);
                    continue;
                }
                if (level_is_solid(tile)) {
                    next_x = fx_from_int((tx + 1) * LEVEL_TILE_SIZE);
                    player->vx = 0;
                    if (player->hand_state == HAND_DASH) {
                        player->hand_state = HAND_NONE;
                        player->hand_timer = 0;
                    }
                    blocked = 1;
                    break;
                }
            }
            if (blocked) break;
        }
    }

    player->x = next_x;
}

static void move_vertical(Player *player, Level *level)
{
    fx32 next_y = player->y + player->vy;
    int new_y = fx_floor_pixel(next_y);
    int left = fx_floor_pixel(player->x);
    int right = left + player->width - 1;
    int x, ty;

    player->grounded = 0;

    if (player->vy >= 0) {
        int old_bottom = fx_floor_pixel(player->y + fx_from_int(player->height));
        int new_bottom = fx_floor_pixel(next_y + fx_from_int(player->height));
        int old_ty = tile_floor_pixel(old_bottom);
        int new_ty = tile_floor_pixel(new_bottom);
        int first_ty = new_ty > old_ty ? old_ty + 1 : new_ty;
        for (ty = first_ty; ty <= new_ty; ++ty) {
            int blocked = 0;
            for (x = left + 1; x <= right - 1; x += 5) {
                int tx = tile_floor_pixel(x);
                uint8_t tile = level_get_tile(level, tx, ty);
                if (((tile == TILE_BREAKABLE || tile == TILE_BREAKABLE_ONEWAY) &&
                     (player->hand_state == HAND_GROUND_POUND || player->swole)) ||
                    (tile == TILE_SWOLVY_BLOCK && player->swole)) {
                    level_break_tile(level, tx, ty);
                    continue;
                }
                if (tile == TILE_ONEWAY || tile == TILE_BREAKABLE_ONEWAY) {
                    int platform_top = ty * LEVEL_TILE_SIZE;
                    if (old_bottom <= platform_top && new_bottom >= platform_top) {
                        next_y = fx_from_int(platform_top - player->height);
                        player->vy = 0;
                        player->grounded = 1;
                        blocked = 1;
                        break;
                    }
                    continue;
                }
                if (level_is_solid(tile)) {
                    next_y = fx_from_int(ty * LEVEL_TILE_SIZE - player->height);
                    player->vy = 0;
                    player->grounded = 1;
                    if (player->hand_state == HAND_GROUND_POUND) {
                        player->hand_state = HAND_NONE;
                        player->hand_timer = 0;
                    }
                    blocked = 1;
                    break;
                }
            }
            if (blocked) break;
        }
    } else {
        int old_ty = tile_floor_pixel(fx_floor_pixel(player->y));
        int new_ty = tile_floor_pixel(new_y);
        int first_ty = new_ty < old_ty ? old_ty - 1 : new_ty;
        for (ty = first_ty; ty >= new_ty; --ty) {
            int blocked = 0;
            for (x = left + 1; x <= right - 1; x += 5) {
                int tx = tile_floor_pixel(x);
                uint8_t tile = level_get_tile(level, tx, ty);
                if (level_is_solid(tile)) {
                    next_y = fx_from_int((ty + 1) * LEVEL_TILE_SIZE);
                    player->vy = 0;
                    blocked = 1;
                    break;
                }
            }
            if (blocked) break;
        }
    }
    player->y = next_y;
}

void player_init(Player *player, int x, int y)
{
    player->x = fx_from_int(x);
    player->y = fx_from_int(y);
    player->vx = 0;
    player->vy = 0;
    player->width = PLAYER_NORMAL_W;
    player->height = PLAYER_NORMAL_H;
    player->origin_x = PLAYER_NORMAL_ORIGIN_X;
    player->origin_y = PLAYER_NORMAL_ORIGIN_Y;
    player->facing = 1;
    player->grounded = 1;
    player->coyote_frames = 6;
    player->jump_buffer_frames = 0;
    player->onions = 0;
    player->swole = 0;
    player->hover_energy = 60;
    player->hand_state = HAND_NONE;
    player->hand_timer = 0;
    player->invulnerability = 0;
    player->dead = 0;
    player->native_scale = 0;
    player->enemies_defeated = 0;
    player->spring_activations = 0;
}

void player_init_native(Player *player, int x, int y)
{
    player_init(player, x, y);
    player->native_scale = 1;
    player->width = PLAYER_NATIVE_NORMAL_W;
    player->height = PLAYER_NATIVE_NORMAL_H;
    player->origin_x = PLAYER_NATIVE_NORMAL_ORIGIN_X;
    player->origin_y = PLAYER_NATIVE_NORMAL_ORIGIN_Y;
}

void player_update(Player *player, const InputState *input, Level *level)
{
    int left = (input->held & KEY_LEFT) != 0;
    int right = (input->held & KEY_RIGHT) != 0;
    int up = (input->held & KEY_UP) != 0;
    int down = (input->held & KEY_DOWN) != 0;
    int hand_pressed = (input->pressed & KEY_B) != 0;
    int jump_pressed = (input->pressed & KEY_A) != 0;
    int grinding = (input->held & KEY_R) != 0;

    if (player->dead) return;

    if (player->invulnerability) --player->invulnerability;
    if (player->hand_timer) --player->hand_timer;

    if (jump_pressed) player->jump_buffer_frames = 8;
    else if (player->jump_buffer_frames) --player->jump_buffer_frames;

    if (player->grounded) {
        player->coyote_frames = 6;
        player->hover_energy = 60;
    } else if (player->coyote_frames) {
        --player->coyote_frames;
    }

    if (left) player->facing = -1;
    if (right) player->facing = 1;

    /* R attaches to the rail and preserves the original horizontal momentum. */
    if (grinding && rail_tile_y(player, level) >= 0 &&
        player->hand_state != HAND_GROUND_POUND) {
        int rail_y = rail_tile_y(player, level);
        player->y = fx_from_int(rail_y * LEVEL_TILE_SIZE - player->height);
        if (player->vx == 0) player->vx = player->facing * physics_scale(player, GRIND_SPEED);
        player->hand_state = HAND_GRIND;
        player->vy = 0;
        player->grounded = 0;
    } else if (player->hand_state == HAND_GRIND &&
               (!grinding || rail_tile_y(player, level) < 0)) {
        player->hand_state = HAND_NONE;
    }

    /* Big Hand action selection; mappings are provisional until video review. */
    if (hand_pressed && down) {
        if (!player->grounded) {
            player->hand_state = HAND_GROUND_POUND;
            player->hand_timer = 30;
            player->vy = physics_scale(player, GROUND_POUND_SPEED);
            player->vx = player->vx / 2;
        } else {
            try_ground_dig(player, level);
        }
    } else if (hand_pressed && up) {
        if (!player->grounded) {
            player->hand_state = HAND_DASH_UP;
            player->hand_timer = 20;
            player->vx = 0;
            player->vy = physics_scale(player, DASH_UP_SPEED);
        } else {
            player->hand_state = HAND_PUNCH_UP;
            player->hand_timer = 10;
            try_punch_block(player, level, 1);
            player->vy = physics_scale(player, PUNCH_UP_SPEED);
        }
    } else if (hand_pressed && (left || right)) {
        player->hand_state = HAND_DASH;
        player->hand_timer = 20;
        player->vx = player->facing * physics_scale(player, DASH_SPEED);
        player->vy = 0;
    } else if (hand_pressed && player->grounded) {
        player->hand_state = HAND_PUNCH;
        player->hand_timer = 10;
        try_punch_block(player, level, 0);
    }

    /* Trace-derived jump buffer: source tuning records jump_buffer=8. */
    if (player->jump_buffer_frames &&
        (player->grounded || player->coyote_frames)) {
        player->vy = physics_scale(player, player->swole ? SWOLVY_JUMP : JUMP_SPEED);
        player->grounded = 0;
        player->coyote_frames = 0;
        player->jump_buffer_frames = 0;
        if (player->hand_state != HAND_DASH) {
            player->hand_state = HAND_NONE;
        }
    }

    if (player->hand_state == HAND_GRIND) {
        if (left || right) {
            player->vx += player->facing * physics_scale(player, GRIND_ACCEL);
        }
        player->vx = fx_clamp(player->vx, -physics_scale(player, GRIND_MAX_SPEED), physics_scale(player, GRIND_MAX_SPEED));
        player->vy = 0;
    } else if (player->hand_state == HAND_DASH) {
        if (player->hand_timer == 0) player->hand_state = HAND_NONE;
    } else if (player->hand_state == HAND_DASH_UP) {
        if (player->hand_timer == 0) {
            player->hand_state = HAND_NONE;
        } else {
            player->vx = 0;
            player->vy = physics_scale(player, DASH_UP_SPEED);
        }
    } else if (player->hand_state == HAND_GROUND_POUND) {
        player->vy = physics_scale(player, GROUND_POUND_SPEED);
    } else if (player->hand_state == HAND_DIG) {
        player->vx = player->facing * physics_scale(player, DIG_SPEED);
        player->vy = 0;
        if (player->hand_timer == 0) player->hand_state = HAND_NONE;
    } else {
        fx32 max_speed = physics_scale(player, player->swole ? (FX_ONE * 5 / 4) : MOVE_MAX_SPEED);

        if (left && !right) {
            player->vx -= physics_scale(player, MOVE_ACCEL);
        } else if (right && !left) {
            player->vx += physics_scale(player, MOVE_ACCEL);
        } else if (player->vx > 0) {
            player->vx -= physics_scale(player, MOVE_FRICTION);
            if (player->vx < 0) player->vx = 0;
        } else if (player->vx < 0) {
            player->vx += physics_scale(player, MOVE_FRICTION);
            if (player->vx > 0) player->vx = 0;
        }

        player->vx = fx_clamp(player->vx, -max_speed, max_speed);

        if (!player->grounded) {
            if ((input->held & KEY_B) && !down &&
                player->hover_energy && player->hand_state != HAND_GROUND_POUND) {
                player->hand_state = HAND_HOVER;
                if (player->vy > physics_scale(player, HOVER_FALL_SPEED)) {
                    player->vy = physics_scale(player, HOVER_FALL_SPEED);
                }
                --player->hover_energy;
            } else {
                if (player->hand_state == HAND_HOVER) {
                    player->hand_state = HAND_NONE;
                }
                player->vy += physics_scale(player, GRAVITY);
                player->vy = fx_clamp(player->vy, -FX_ONE * 4,
                                      physics_scale(player, MAX_FALL_SPEED));
            }
        } else if (player->hand_state == HAND_HOVER) {
            player->hand_state = HAND_NONE;
        }
    }

    if (player->hand_timer == 0 &&
        (player->hand_state == HAND_PUNCH ||
         player->hand_state == HAND_PUNCH_UP)) {
        player->hand_state = HAND_NONE;
    }

    move_horizontal(player, level);
    move_vertical(player, level);

    if (touching_hazard(player, level) && !player->invulnerability) {
        player->dead = 1;
    }

    collect_onions(player, level);

    if (fx_to_int(player->y) > level_height_pixels(level) + 24) {
        player->dead = 1;
    }
}
