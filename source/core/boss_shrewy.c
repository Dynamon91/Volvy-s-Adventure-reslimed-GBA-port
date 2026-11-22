#include "core/boss_shrewy.h"

#define SHREWY_ORIGIN_X 25
#define SHREWY_ORIGIN_Y 39
#define SHREWY_WIDTH 48
#define SHREWY_HEIGHT 48
#define SHREWY_GROUND_PIVOT_Y 104
#define SHREWY_LEFT_LIMIT 32
#define SHREWY_RIGHT_LIMIT 224

/* All timing and damage values in this first controller are provisional.
 * The source sprites and room placements are exact; original GML behavior is
 * not yet decompiled or calibrated against a complete boss-fight recording. */
#define SHREWY_STARTUP_FRAMES 45
#define SHREWY_WINDUP_FRAMES 18
#define SHREWY_RECOVER_FRAMES 26
#define SHREWY_HURT_FRAMES 18
#define SHREWY_BOSS_HEALTH 3
#define SHREWY_PLAYER_HITBOX_IFRAMES 60

static int overlap(int ax, int ay, int aw, int ah,
                   int bx, int by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

static void boss_rect(const ShrewyBoss *boss,
                      int *left, int *top, int *width, int *height)
{
    if (left) *left = fx_to_int(boss->x) - SHREWY_ORIGIN_X;
    if (top) *top = fx_to_int(boss->y) - SHREWY_ORIGIN_Y;
    if (width) *width = SHREWY_WIDTH;
    if (height) *height = SHREWY_HEIGHT;
}

static int player_attack_rect(const Player *player,
                              int *left, int *top, int *width, int *height)
{
    int x = fx_to_int(player->x);
    int y = fx_to_int(player->y);
    switch ((HandState)player->hand_state) {
    case HAND_PUNCH:
        *left = player->facing > 0 ? x + player->width : x - 12;
        *top = y + 2;
        *width = 12;
        *height = player->height - 4;
        return 1;
    case HAND_PUNCH_UP:
        *left = x + 3;
        *top = y - 12;
        *width = player->width - 6;
        *height = 12;
        return 1;
    case HAND_DASH:
    case HAND_DASH_UP:
    case HAND_GROUND_POUND:
    case HAND_DIG:
        *left = x - 3;
        *top = y - 3;
        *width = player->width + 6;
        *height = player->height + 6;
        return 1;
    default:
        return 0;
    }
}

static int player_rect_hits_boss(const Player *player,
                                 const ShrewyBoss *boss)
{
    int bx, by, bw, bh;
    boss_rect(boss, &bx, &by, &bw, &bh);
    return overlap(fx_to_int(player->x), fx_to_int(player->y),
                   player->width, player->height, bx, by, bw, bh);
}

static int boss_is_vulnerable(const ShrewyBoss *boss)
{
    return boss->state == SHREWY_IDLE || boss->state == SHREWY_RECOVER;
}

static void boss_take_hit(ShrewyBoss *boss)
{
    if (boss->invulnerability || boss->state == SHREWY_DEFEATED) return;
    if (boss->health) --boss->health;
    boss->invulnerability = 28;
    boss->rock_active = 0;
    boss->vx = 0;
    boss->vy = 0;
    if (!boss->health) {
        boss->state = SHREWY_DEFEATED;
        boss->timer = 0;
    } else {
        boss->state = SHREWY_HURT;
        boss->timer = SHREWY_HURT_FRAMES;
    }
}

static void player_take_hit(Player *player)
{
    if (player->invulnerability || player->dead) return;
    player->dead = 1;
    player->invulnerability = SHREWY_PLAYER_HITBOX_IFRAMES;
}

static void start_attack(ShrewyBoss *boss, const Player *player)
{
    int player_center = fx_to_int(player->x) + player->width / 2;
    int boss_center = fx_to_int(boss->x);
    boss->facing = player_center < boss_center ? 0 : 1;
    boss->vx = 0;
    boss->vy = 0;
    boss->state = SHREWY_WINDUP;
    boss->timer = SHREWY_WINDUP_FRAMES;
}

static void spawn_rock(ShrewyBoss *boss, const Player *player)
{
    int direction = boss->facing ? 1 : -1;
    int player_x = fx_to_int(player->x) + player->width / 2;
    if ((direction > 0 && player_x < fx_to_int(boss->x)) ||
        (direction < 0 && player_x > fx_to_int(boss->x))) {
        direction = -direction;
    }
    boss->rock_x = boss->x + fx_from_int(direction * 28);
    boss->rock_y = boss->y - fx_from_int(24);
    boss->rock_vx = fx_from_int(direction * 3);
    boss->rock_vy = -FX_ONE;
    boss->rock_timer = 90;
    boss->rock_active = 1;
}

static void update_rock(ShrewyBoss *boss, Player *player)
{
    int rx, ry;
    if (!boss->rock_active) return;
    boss->rock_x += boss->rock_vx;
    boss->rock_y += boss->rock_vy;
    boss->rock_vy += FX_ONE / 8;
    if (boss->rock_timer) --boss->rock_timer;
    rx = fx_to_int(boss->rock_x) - 13;
    ry = fx_to_int(boss->rock_y) - 13;
    if (!boss->rock_timer || rx > 256 || rx + 27 < 0 || ry > 144) {
        boss->rock_active = 0;
        return;
    }
    if (overlap(rx, ry, 27, 26, fx_to_int(player->x),
                fx_to_int(player->y), player->width, player->height)) {
        player_take_hit(player);
        boss->rock_active = 0;
    }
}

void shrewy_init(ShrewyBoss *boss)
{
    boss->x = fx_from_int(184); /* exact source rm_shrewy boss pivot */
    boss->y = fx_from_int(104);
    boss->vx = 0;
    boss->vy = 0;
    boss->rock_x = 0;
    boss->rock_y = 0;
    boss->rock_vx = 0;
    boss->rock_vy = 0;
    boss->health = SHREWY_BOSS_HEALTH; /* test value; original HP not decoded */
    boss->state = SHREWY_IDLE;
    boss->timer = SHREWY_STARTUP_FRAMES;
    boss->invulnerability = 0;
    boss->facing = 0;
    boss->next_attack = 0;
    boss->rock_active = 0;
    boss->rock_timer = 0;
}

void shrewy_update(ShrewyBoss *boss, Player *player,
                   int pre_bottom, int was_descending)
{
    int bx, by, bw, bh;
    int attack_x, attack_y, attack_w, attack_h;
    int has_attack;
    int px = fx_to_int(player->x);
    int py = fx_to_int(player->y);
    int bottom = py + player->height;

    if (boss->state == SHREWY_DEFEATED || player->dead) return;
    if (boss->invulnerability) --boss->invulnerability;
    update_rock(boss, player);
    if (player->dead) return;

    boss_rect(boss, &bx, &by, &bw, &bh);
    has_attack = player_attack_rect(player, &attack_x, &attack_y,
                                    &attack_w, &attack_h);

    if (!boss->invulnerability && boss_is_vulnerable(boss)) {
        if ((has_attack && overlap(attack_x, attack_y, attack_w, attack_h,
                                   bx, by, bw, bh)) ||
            (was_descending && pre_bottom <= by + 4 && bottom >= by &&
             overlap(px, py, player->width, player->height,
                     bx, by, bw, bh))) {
            boss_take_hit(boss);
            if (was_descending && pre_bottom <= by + 4) {
                player->y = fx_from_int(by - player->height);
                player->vy = -FX_ONE * 2;
                player->grounded = 0;
            }
            return;
        }
    }

    switch ((ShrewyState)boss->state) {
    case SHREWY_IDLE:
        if (boss->timer) --boss->timer;
        if (!boss->timer) start_attack(boss, player);
        break;

    case SHREWY_WINDUP:
        if (boss->timer) --boss->timer;
        if (!boss->timer) {
            switch (boss->next_attack % 3u) {
            case 0:
                boss->state = SHREWY_JUMP;
                boss->vx = fx_from_int(boss->facing ? 1 : -1);
                boss->vy = -FX_ONE * 4;
                boss->timer = 60;
                break;
            case 1:
                boss->state = SHREWY_DROPKICK;
                boss->vx = fx_from_int(boss->facing ? 3 : -3);
                boss->timer = 24;
                break;
            default:
                boss->state = SHREWY_TORNADO;
                boss->timer = 48;
                break;
            }
            ++boss->next_attack;
        }
        break;

    case SHREWY_JUMP:
        boss->x += boss->vx;
        boss->y += boss->vy;
        boss->vy += FX_ONE / 5;
        if (fx_to_int(boss->x) < SHREWY_LEFT_LIMIT)
            boss->x = fx_from_int(SHREWY_LEFT_LIMIT);
        if (fx_to_int(boss->x) > SHREWY_RIGHT_LIMIT)
            boss->x = fx_from_int(SHREWY_RIGHT_LIMIT);
        if (fx_to_int(boss->y) >= SHREWY_GROUND_PIVOT_Y) {
            boss->y = fx_from_int(SHREWY_GROUND_PIVOT_Y);
            boss->vy = 0;
            boss->state = SHREWY_STOMP;
            boss->timer = 20;
        }
        break;

    case SHREWY_STOMP:
        if (player_rect_hits_boss(player, boss)) player_take_hit(player);
        if (boss->timer) --boss->timer;
        if (!boss->timer) {
            boss->state = SHREWY_RECOVER;
            boss->timer = SHREWY_RECOVER_FRAMES;
        }
        break;

    case SHREWY_DROPKICK:
        boss->x += boss->vx;
        if (fx_to_int(boss->x) < SHREWY_LEFT_LIMIT)
            boss->x = fx_from_int(SHREWY_LEFT_LIMIT);
        if (fx_to_int(boss->x) > SHREWY_RIGHT_LIMIT)
            boss->x = fx_from_int(SHREWY_RIGHT_LIMIT);
        if (player_rect_hits_boss(player, boss)) player_take_hit(player);
        if (boss->timer) --boss->timer;
        if (!boss->timer) {
            boss->state = SHREWY_RECOVER;
            boss->timer = SHREWY_RECOVER_FRAMES;
        }
        break;

    case SHREWY_TORNADO:
        if (boss->timer == 32 && !boss->rock_active)
            spawn_rock(boss, player);
        if (boss->timer) --boss->timer;
        if (player_rect_hits_boss(player, boss)) player_take_hit(player);
        if (!boss->timer) {
            boss->state = SHREWY_RECOVER;
            boss->timer = SHREWY_RECOVER_FRAMES;
        }
        break;

    case SHREWY_HURT:
        if (boss->timer) --boss->timer;
        if (!boss->timer) {
            boss->state = SHREWY_RECOVER;
            boss->timer = SHREWY_RECOVER_FRAMES;
        }
        break;

    case SHREWY_RECOVER:
        if (boss->timer) --boss->timer;
        if (!boss->timer) {
            boss->state = SHREWY_IDLE;
            boss->timer = 36;
        }
        break;

    case SHREWY_DEFEATED:
    default:
        break;
    }
}

int shrewy_is_defeated(const ShrewyBoss *boss)
{
    return boss->state == SHREWY_DEFEATED || boss->health == 0;
}
