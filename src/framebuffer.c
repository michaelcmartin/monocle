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
static int hide_mouse = 0;

int
mncl_config_video(int width, int height, int fullscreen, int flags)
{
    screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE | SDL_ANYFORMAT | SDL_DOUBLEBUF | (fullscreen ? SDL_FULLSCREEN : 0));
    if (!screen) {
        return 1;
    }
    is_fullscreen = fullscreen;
    clear_color = SDL_MapRGB(screen->format, 0, 0, 0);
    SDL_ShowCursor((fullscreen && hide_mouse) ? SDL_DISABLE : SDL_ENABLE);
    /* TODO: We should also normalize all the spritesheets we know
     * about, but we can't do that until the resource manager is
     * done */
    return 0;
}

int
mncl_is_fullscreen(void)
{
    return is_fullscreen;
}

int
mncl_toggle_fullscreen(void)
{
    /* We are not guaranteed that changing fullscreen status keeps the
     * colormap the same, so we will need to recompute the clear
     * color. */
    Uint8 r, g, b;
    int w, h;
    if (!screen) {
        return is_fullscreen;
    }
    SDL_GetRGB(clear_color, screen->format, &r, &g, &b);
    w = screen->w;
    h = screen->h;
    is_fullscreen = 1-is_fullscreen;
    mncl_config_video(w, h, is_fullscreen, 0);
    if (!screen) {
        /* Revert if it didn't work. This should never realistically fail. */
        is_fullscreen = 1-is_fullscreen;
        mncl_config_video(w, h, is_fullscreen, 0);
    }
    if (screen) {
        mncl_set_clear_color(r, g, b);
    }

    return is_fullscreen;
}

void
mncl_set_clear_color(unsigned char r, unsigned char g, unsigned char b)
{
    clear_color = SDL_MapRGB(screen->format, r, g, b);
}

void
mncl_hide_mouse_in_fullscreen(int val)
{
    hide_mouse = val;
    if (screen && is_fullscreen) {
        SDL_ShowCursor(val ? SDL_DISABLE : SDL_ENABLE);
    }
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
               unsigned char r, unsigned char g, unsigned char b)
{
    SDL_Rect rect;
    Uint32 col;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    col = SDL_MapRGB(screen->format, r, g, b);
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
    spritesheet->used = NULL;
    mncl_normalize_spritesheet(spritesheet);
    mncl_release_raw(raw);
    return spritesheet;
}

void
mncl_normalize_spritesheet(MNCL_SPRITESHEET *spritesheet)
{
    if (!spritesheet) {
        /* Can't normalize if there's nothing to normalize */
        return;
    }
    if (!screen) {
        /* Can't normalize if there's nothing to normalize to */
        return;
    }
    if (spritesheet->used && spritesheet->used != spritesheet->core) {
        SDL_FreeSurface(spritesheet->used);
    }
    spritesheet->used = SDL_DisplayFormatAlpha(spritesheet->core);
    if (!spritesheet->used) {
        spritesheet->used = spritesheet->core;
    }
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

MNCL_SPRITE *
mncl_alloc_sprite(int nframes)
{
    size_t size = sizeof(MNCL_SPRITE) + sizeof(MNCL_FRAME) * nframes;
    MNCL_SPRITE *result = (MNCL_SPRITE *)malloc(size);
    if (!result) {
        return NULL;
    }
    memset(result, 0, size);
    result->nframes = nframes;
    return result;
}

void
mncl_free_sprite(MNCL_SPRITE *sprite)
{
    if (sprite) {
        free(sprite);
    }
}

void
mncl_draw_sprite(MNCL_SPRITE *s, int x, int y, int frame)
{
    MNCL_FRAME *f;
    if (!s) {
        return;
    }
    frame %= s->nframes;
    if (frame < 0) {
        frame += s->nframes;
    }
    f = s->frames + frame;
    mncl_draw_from_spritesheet(f->sheet, x-s->hot_x, y-s->hot_y, f->x, f->y, s->w, s->h);
}
