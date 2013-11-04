#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include "monocle.h"
#include "monocle_internal.h"

/* This file contains the framebuffer component and the spritesheet
 * component, since both need to talk to screen, and nobody else
 * does. */

/* The basic framebuffer routines */

static SDL_Window *screen = NULL;
static SDL_Renderer *renderer = NULL;
static Uint8 clear_color_r = 0, clear_color_g = 0, clear_color_b = 0;
static int is_fullscreen = 0, win_width = 0, win_height = 0;
static int hide_mouse = 0;

int
mncl_config_video(const char *title, int width, int height, int fullscreen, int flags)
{
    if (!screen) {
        screen = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        if (!screen) {
            return 1;
        }
        renderer = SDL_CreateRenderer(screen, -1, 0);
        if (!renderer) {
            SDL_DestroyWindow(screen);
            screen = NULL;
            return 1;
        }
        mncl_begin_frame();
        mncl_end_frame();        
    } else {
        SDL_SetWindowFullscreen(screen, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        if (!fullscreen) {
            SDL_SetWindowSize (screen, width, height);
        }
    }        
    SDL_RenderSetLogicalSize(renderer, width, height);
    is_fullscreen = fullscreen;
    clear_color_r = clear_color_g = clear_color_b = 0;
    win_width = width;
    win_height = height;
    SDL_ShowCursor((fullscreen && hide_mouse) ? SDL_DISABLE : SDL_ENABLE);
    mncl_renormalize_all_spritesheets();
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
    if (!screen) {
        return is_fullscreen;
    }
    is_fullscreen = 1-is_fullscreen;
    SDL_SetWindowFullscreen(screen, is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    SDL_ShowCursor((is_fullscreen && hide_mouse) ? SDL_DISABLE : SDL_ENABLE);
    if (!is_fullscreen) {
        SDL_SetWindowSize(screen, win_width, win_height);
    }

    return is_fullscreen;
}

void
mncl_set_clear_color(unsigned char r, unsigned char g, unsigned char b)
{
    clear_color_r = r;
    clear_color_g = g;
    clear_color_b = b;
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
    SDL_SetRenderDrawColor(renderer, clear_color_r, clear_color_g, clear_color_b, 255);
    SDL_RenderClear(renderer);
}

void
mncl_end_frame(void)
{
    SDL_RenderPresent(renderer);
}

void
mncl_draw_rect(int x, int y, int w, int h, 
               unsigned char r, unsigned char g, unsigned char b)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &rect);
}

struct struct_MNCL_SPRITESHEET {
    SDL_Texture *tex;
};

MNCL_SPRITESHEET *
mncl_alloc_spritesheet(const char *resource_name)
{
    MNCL_SPRITESHEET *spritesheet;
    MNCL_RAW *raw;
    SDL_Surface *loaded;

    if (!renderer) {
        /* TODO: Make this requirement not hold */
        fprintf(stderr, "ERROR: Don't load spritesheets before configuring video\n");
        return NULL;
    }

    raw = mncl_acquire_raw(resource_name);
    if (!raw) {
        return NULL;
    }

    spritesheet = (MNCL_SPRITESHEET *)malloc(sizeof(MNCL_SPRITESHEET));
    if (!spritesheet) {
        return NULL;
    }
    loaded = IMG_Load_RW(SDL_RWFromMem(raw->data, raw->size), 1);
    if (!loaded) {
        free(spritesheet);
        mncl_release_raw(raw);
        return NULL;
    }
    spritesheet->tex = SDL_CreateTextureFromSurface(renderer, loaded);
    if (!spritesheet->tex) {
        free(spritesheet);
        spritesheet = NULL;
        printf ("Failed to make a texture for %s\n", resource_name);
        /* Fall through to cleanup */
    } else {
        printf ("Successfully made a texture for %s\n", resource_name);
    }
    SDL_FreeSurface(loaded);
    mncl_release_raw(raw);
    return spritesheet;
}

void
mncl_normalize_spritesheet(MNCL_SPRITESHEET *spritesheet)
{
    /* This might do the work of actually setting up the texture,
     * maybe? */
}

void
mncl_free_spritesheet(MNCL_SPRITESHEET *spritesheet)
{
    if (!spritesheet) {
        return;
    }
    SDL_DestroyTexture(spritesheet->tex);
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
    dest.w = my_w;
    dest.h = my_h;
    src.x = my_x;
    src.y = my_y;
    src.w = my_w;
    src.h = my_h;
    SDL_RenderCopy(renderer, spritesheet->tex, &src, &dest);
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
