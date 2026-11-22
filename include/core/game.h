#ifndef VOLVY_CORE_GAME_H
#define VOLVY_CORE_GAME_H

typedef enum GameMode {
    GAME_TITLE = 0,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER,
    GAME_VICTORY,
    GAME_BOSS,
    GAME_BONUS,
    GAME_CARD_PREVIEW /* Optional developer-only reference viewer. */
} GameMode;

void game_init(void);
void game_update(void);
void game_render(void);
void game_vblank_commit(void);

#endif
