#include <gba.h>
#include <string.h>
#include "platform/ui.h"

#define UI_FONT_COUNT 40
#define UI_SCREENBLOCK 29
#define UI_MAP_VRAM ((u16 *)0x0600E800)
#define UI_FONT_VRAM ((u8 *)0x06000000)
#define UI_PALETTE_BANK 15

typedef struct Glyph5x5 { char code; u8 bits[5]; } Glyph5x5;
static const Glyph5x5 s_glyphs[] = {
    {'A',{14,17,31,17,17}}, {'B',{30,17,30,17,30}},
    {'C',{15,16,16,16,15}}, {'D',{30,17,17,17,30}},
    {'E',{31,16,30,16,31}}, {'F',{31,16,30,16,16}},
    {'G',{15,16,23,17,15}}, {'H',{17,17,31,17,17}},
    {'I',{31,4,4,4,31}}, {'J',{7,2,2,18,12}},
    {'K',{17,18,28,18,17}}, {'L',{16,16,16,16,31}},
    {'M',{17,27,21,17,17}}, {'N',{17,25,21,19,17}},
    {'O',{14,17,17,17,14}}, {'P',{30,17,30,16,16}},
    {'Q',{14,17,21,18,13}}, {'R',{30,17,30,18,17}},
    {'S',{15,16,14,1,30}}, {'T',{31,4,4,4,4}},
    {'U',{17,17,17,17,14}}, {'V',{17,17,17,10,4}},
    {'W',{17,17,21,27,17}}, {'X',{17,10,4,10,17}},
    {'Y',{17,10,4,4,4}}, {'Z',{31,2,4,8,31}},
    {'0',{14,19,21,25,14}}, {'1',{4,12,4,4,14}},
    {'2',{14,17,2,4,31}}, {'3',{30,1,6,1,30}},
    {'4',{2,6,10,31,2}}, {'5',{31,16,30,1,30}},
    {'6',{15,16,30,17,14}}, {'7',{31,1,2,4,4}},
    {'8',{14,17,14,17,14}}, {'9',{14,17,15,1,30}},
    {'-',{0,0,31,0,0}}, {':',{0,4,0,4,0}},
    {' ',{0,0,0,0,0}}
};

static u16 s_ui_map[32*32] ALIGN(4);
static u8 s_font_tiles[UI_FONT_COUNT*32] ALIGN(4);
static GameMode s_last_mode = (GameMode)255;
static uint16_t s_last_onions = 0xFFFF;
static uint8_t s_last_stage = 0;
static uint8_t s_last_boss_health = 0xFF;
static uint32_t s_last_timer_seconds = 0xFFFFFFFFu;
static u8 s_dirty;

static unsigned int glyph_index(char ch)
{
    unsigned int i;
    for (i=0; i<sizeof(s_glyphs)/sizeof(s_glyphs[0]); ++i) {
        if (s_glyphs[i].code==ch) return i+1;
    }
    return 0;
}

static void font_generate(void)
{
    unsigned int i, x, y;
    memset(s_font_tiles,0,sizeof(s_font_tiles));
    for (i=0; i<sizeof(s_glyphs)/sizeof(s_glyphs[0]); ++i) {
        /* Tile index i+1; tile 0 is the all-transparent blank. */
        u8 *dst = s_font_tiles+(i+1)*32;
        for (y=0;y<5;++y) {
            for (x=0;x<5;++x) {
                if (s_glyphs[i].bits[y]&(1u << (4-x))) {
                    int pos=(int)((y+1)*8+(x+1));
                    dst[pos>>1] |= (u8)(1u << ((pos&1)*4));
                }
            }
        }
    }
}

static void draw_str(int col, int row, const char *message)
{
    while (*message && col<30) {
        if (col>=0 && row>=0 && row<20) {
            s_ui_map[row*32+col]=(u16)(glyph_index(*message)|(UI_PALETTE_BANK<<12));
        }
        ++col; ++message;
    }
}

void gba_ui_init(void)
{
    font_generate();
    /* 40 tile indices including blank -> 1280 bytes, fits character block 0. */
    dmaCopy(s_font_tiles,UI_FONT_VRAM,sizeof(s_font_tiles));
    BGCTRL[0]=BG_SIZE_0|CHAR_BASE(0)|SCREEN_BASE(UI_SCREENBLOCK)|0;
    BG_OFFSET[0].x=0; BG_OFFSET[0].y=0;
    BG_COLORS[UI_PALETTE_BANK*16+1]=0x7FFF;
    s_last_mode=(GameMode)255;
    s_last_onions=0xFFFF;
    s_last_stage=0;
    s_last_boss_health=0xFF;
    s_last_timer_seconds=0xFFFFFFFFu;
    s_dirty=1;
}

void gba_ui_update(GameMode mode, uint16_t onions, uint8_t stage,
                   uint8_t boss_health, uint32_t timer_frames)
{
    char count[]="000";
    char boss_count[]="0";
    char time_text[]="00:00";
    uint32_t total_seconds = timer_frames / 60u;
    uint32_t minutes = total_seconds / 60u;
    uint32_t seconds = total_seconds % 60u;
    if (minutes > 99u) minutes = 99u;
    if (mode==s_last_mode && onions==s_last_onions && stage==s_last_stage &&
        boss_health==s_last_boss_health && total_seconds==s_last_timer_seconds) return;
    s_last_mode=mode;
    s_last_onions=onions;
    s_last_stage=stage;
    s_last_boss_health=boss_health;
    s_last_timer_seconds=total_seconds;
    memset(s_ui_map,0,sizeof(s_ui_map));
    if (mode==GAME_TITLE) {
        draw_str(12,5,"VOLVY");
        draw_str(11,7,"RESLIMED");
        draw_str(9,13,"PRESS START");
        draw_str(8,15,"SELECT LEVEL");
        draw_str(11,17,stage==5 ? "BONUS RACE" :
                         stage==4 ? "SHREWY" :
                         stage==3 ? "W1-L3" : stage==2 ? "W1-L2" : "W1-L1");
    } else if (mode==GAME_PLAYING || mode==GAME_PAUSED || mode==GAME_BOSS || mode==GAME_BONUS) {
        if (stage==4 || mode==GAME_BOSS) {
            boss_count[0]=(char)('0'+(boss_health>9 ? 9 : boss_health));
            draw_str(1,1,"SHREWY");
            draw_str(22,1,"HP");
            draw_str(25,1,boss_count);
            draw_str(7,18,"HAND ATTACK");
        } else if (stage==5 || mode==GAME_BONUS) {
            time_text[0]=(char)('0'+(minutes/10u));
            time_text[1]=(char)('0'+(minutes%10u));
            time_text[3]=(char)('0'+(seconds/10u));
            time_text[4]=(char)('0'+(seconds%10u));
            draw_str(1,1,"RACE");
            draw_str(21,1,time_text);
            if (onions>999) onions=999;
            count[0]=(char)('0'+onions/100);
            count[1]=(char)('0'+(onions/10)%10);
            count[2]=(char)('0'+onions%10);
            draw_str(1,18,"ONIONS");
            draw_str(8,18,count);
        } else {
            if (onions>999) onions=999;
            count[0]=(char)('0'+onions/100);
            count[1]=(char)('0'+(onions/10)%10);
            count[2]=(char)('0'+onions%10);
            draw_str(1,1,"ONIONS");
            draw_str(8,1,count);
            draw_str(22,1,stage==3 ? "W1-L3" : stage==2 ? "W1-L2" : "W1-L1");
        }
        if (mode==GAME_PAUSED) {
            draw_str(12,8,"PAUSED");
            draw_str(9,10,"PRESS START");
        }
    } else if (mode==GAME_OVER) {
        draw_str(11,8,"GAME OVER");
        draw_str(9,10,"PRESS START");
    } else if (mode==GAME_VICTORY) {
        draw_str(10,8,stage==5 ? "RACE CLEAR" :
                         stage==4 ? "BOSS CLEAR" : "STAGE CLEAR");
        draw_str(9,10,"PRESS START");
    }
    s_dirty=1;
}

void gba_ui_vblank_commit(void)
{
    if (!s_dirty) return;
    dmaCopy(s_ui_map,UI_MAP_VRAM,sizeof(s_ui_map));
    s_dirty=0;
}
