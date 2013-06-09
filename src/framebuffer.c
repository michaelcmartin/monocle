#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include "monocle.h"

/* This file contains the framebuffer component and the spritesheet
 * component, since both need to talk to screen, and nobody else
 * does. */

/* The basic framebuffer routines */

static SDL_Surface *screen = NULL;
static Uint32 clear_color = 0;
static int is_fullscreen = 0;

int
mncl_config_video(int width, int height, int fullscreen, int flags)
{
    screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE | SDL_ANYFORMAT | SDL_DOUBLEBUF | (fullscreen ? SDL_FULLSCREEN : 0));
    if (!screen) {
        return 1;
    }
    is_fullscreen = fullscreen;
    clear_color = SDL_MapRGB(screen->format, 0, 0, 0);
}

int
mncl_is_fullscreen(void)
{
    return is_fullscreen;
}

int
mncl_toggle_fullscreen(void)
{
    // TODO: Just fail at it for now, to heck with fullscreen at first
    return is_fullscreen;
}

void
mncl_set_clear_color(unsigned char r, unsigned char g, unsigned char b)
{
    clear_color = SDL_MapRGB(screen->format, r, g, b);
}

void
mncl_begin_frame(void)
{
    SDL_FillRect(screen, NULL, clear_color);
}

void
mncl_end_frame(void)
{
    SDL_Flip(screen);
}

void
mncl_draw_rect(int x, int y, int w, int h, 
               unsigned char r, unsigned char g, unsigned char b, 
               unsigned char a)
{
    SDL_Rect rect;
    Uint32 col;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    col = SDL_MapRGBA(screen->format, r, g, b, a);
    SDL_FillRect(screen, &rect, col);
}

struct struct_MNCL_SPRITESHEET {
    SDL_Surface *core, *used;
};

MNCL_SPRITESHEET *
mncl_alloc_spritesheet(const char *resource_name)
{
    MNCL_SPRITESHEET *spritesheet;
    MNCL_RAW *raw;

    raw = mncl_acquire_raw(resource_name);
    if (!raw) {
        return NULL;
    }

    spritesheet = (MNCL_SPRITESHEET *)malloc(sizeof(MNCL_SPRITESHEET));
    if (!spritesheet) {
        return NULL;
    }
    spritesheet->core = IMG_Load_RW(SDL_RWFromMem(raw->data, raw->size), 1);
    if (!spritesheet->core) {
        free(spritesheet);
        mncl_release_raw(raw);
        return NULL;
    }
    spritesheet->used = SDL_DisplayFormatAlpha(spritesheet->core);
    if (!spritesheet->used) {
        spritesheet->used = spritesheet->core;
    }
    mncl_release_raw(raw);
    return spritesheet;
}

void
mncl_free_spritesheet(MNCL_SPRITESHEET *spritesheet)
{
    if (!spritesheet) {
        return;
    }
    if (spritesheet->used != spritesheet->core) {
        SDL_FreeSurface(spritesheet->used);
    }
    SDL_FreeSurface(spritesheet->core);
    free(spritesheet);
}

void 
mncl_draw_from_spritesheet(MNCL_SPRITESHEET *spritesheet,
                           int x, int y, 
                           int my_x, int my_y, int my_w, int my_h)
{
    SDL_Rect src, dest;
    dest.x = x;
    dest.y = y;
    src.x = my_x;
    src.y = my_y;
    src.w = my_w;
    src.h = my_h;
    SDL_BlitSurface(spritesheet->used, &src, screen, &dest);
}
