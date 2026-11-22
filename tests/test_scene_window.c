#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "core/scene_window.h"

static uint16_t source[332 * 70];
static uint16_t window_tiles[SCENE_WINDOW_TILES_W * SCENE_WINDOW_TILES_H];

static int lookup(int local_tile_x, int local_tile_y)
{
    assert(local_tile_x >= 0 && local_tile_x < SCENE_WINDOW_TILES_W);
    assert(local_tile_y >= 0 && local_tile_y < SCENE_WINDOW_TILES_H);
    return window_tiles[(local_tile_x / 32) * 1024 +
                        local_tile_y * 32 + (local_tile_x & 31)];
}

static void run_room(int w, int h)
{
    int world_px_w=w*8, world_px_h=h*8;
    int max_cam_x=world_px_w-SCENE_WINDOW_VIEW_W;
    int max_cam_y=world_px_h-SCENE_WINDOW_VIEW_H;
    int camx,camy,px,py,originx,originy;
    int i;
    if(max_cam_x<0)max_cam_x=0;
    if(max_cam_y<0)max_cam_y=0;
    for(i=0;i<w*h;++i) source[i]=(uint16_t)((i % 1000)+1);
    for (camx=0; camx<=max_cam_x; camx+=17) {
        for (camy=0; camy<=max_cam_y; camy+=7) {
            scene_window_origin(camx,camy,w,h,&originx,&originy);
            scene_window_copy(source,w,h,originx,originy,window_tiles);
            assert(camx-originx*8>=0);
            assert(camx-originx*8<=512-SCENE_WINDOW_VIEW_W);
            assert(camy-originy*8>=0);
            assert(camy-originy*8<=256-SCENE_WINDOW_VIEW_H);
            for(py=0;py<SCENE_WINDOW_VIEW_H;py+=13) {
                for(px=0;px<SCENE_WINDOW_VIEW_W;px+=13) {
                    const int world_tx=(camx+px)/8;
                    const int world_ty=(camy+py)/8;
                    const int local_tx=world_tx-originx;
                    const int local_ty=world_ty-originy;
                    const int expected=world_tx<w && world_ty<h ?
                        source[world_ty*w+world_tx] : 0;
                    assert(lookup(local_tx,local_ty)==expected);
                }
            }
        }
    }
    /* Check the exact last camera tile-boundary, not just loop samples. */
    scene_window_origin(max_cam_x,max_cam_y,w,h,&originx,&originy);
    scene_window_copy(source,w,h,originx,originy,window_tiles);
    assert(max_cam_x-originx*8<=272);
    assert(max_cam_y-originy*8<=96);
    {
        const int tx=(max_cam_x+239)/8;
        const int ty=(max_cam_y+159)/8;
        const int actual=lookup(tx-originx,ty-originy);
        if (tx<w && ty<h) assert(actual==source[ty*w+tx]);
        else assert(actual==0);
    }
}

int main(void)
{
    run_room(142,23);  /* World 1 platform scene */
    run_room(332,70);  /* World 1 tall bonus scene */
    run_room(32,18);   /* Shrewy arena, narrower than the BG window */
    puts("64x32 BG window streaming: PASS (horizontal+vertical coverage)");
    return 0;
}
