#include <stdlib.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "monocle.h"

void
instructions(void)
{
    MNCL_DATA *insts;
    insts = mncl_data_resource("instructions");
    if (insts && insts->tag == MNCL_DATA_ARRAY) {
        int i;
        printf("Instructions:\n");
        for (i = 0; i < insts->value.array.size; ++i) {
            MNCL_DATA *val = insts->value.array.data[i];
            printf ("    %s\n", (val && val->tag == MNCL_DATA_STRING) ? val->value.string : "(invalid datum)");
        }
    } else {
        printf("No instructions available\n");
    }
}

void
update(MNCL_OBJECT *obj)
{
    float newx = obj->x + obj->dx;
    float newy = obj->y + obj->dy;
    if (newx < 0) {
        obj->x = 0; obj->dx = -obj->dx;
    }
    if (newy < 0) {
        obj->y = 0; obj->dy = -obj->dy;
    }
    if (newx > 704) {
        obj->x = 704; obj->dx = -obj->dx;
    }
    if (newy > 416) {
        obj->y = 416; obj->dy = -obj->dy;
    }
}

int
main(int argc, char **argv)
{
    MNCL_SFX *sfx;
    MNCL_FONT *monospace;
    int done, i, countdown, music_on, bg, music_volume;

    mncl_init();
    mncl_config_video("Earthball Demo", 768, 480, 0, 0);
    mncl_add_resource_zipfile("earthball-res.zip");

    mncl_load_resmap("earthball.json");
    sfx = mncl_sfx_resource("sfx");
    monospace = mncl_font_resource("monospace");
    mncl_play_music_resource("bgm", 2000);

    for (i = 0; i < 16; ++i) {
        MNCL_OBJECT *obj = mncl_create_object(rand() % (768 - 64), rand() % (480 - 64), "earth");
        obj->dx = (rand() % 5 + 1) * ((rand() % 2) ? 1 : -1);
        obj->dy = (rand() % 5 + 1) * ((rand() % 2) ? 1 : -1);
        obj->f = rand() % 30;
    }

    done = 0;
    countdown = 0;
    music_on = 1;
    bg = 0;
    music_volume = 128;

    instructions();

    mncl_hide_mouse_in_fullscreen(1);

    while (!done) {
        MNCL_EVENT *e = mncl_pop_global_event();
        switch(e->type) {
        case MNCL_EVENT_QUIT:
            done = 1;
            break;
        case MNCL_EVENT_KEYDOWN:
            switch (e->value.key) {
            case SDLK_ESCAPE:
                mncl_fade_out_music(1000);
                countdown = 100;
                break;
            case SDLK_b:
                bg = (bg + 1) % 4;
                mncl_set_clear_color(bg == 1 ? 128 : 0, bg == 2 ? 128 : 0, bg == 3 ? 128 : 0);
                break;
            case SDLK_p:
                music_on = 1 - music_on;
                if (music_on) {
                    mncl_resume_music();
                } else {
                    mncl_pause_music();
                }
                break;
            case SDLK_t:
                printf("Toggled fullscreen to: %s\n", mncl_toggle_fullscreen() ? "on" : "off");
                printf("Fullscreen reported as: %s\n", mncl_is_fullscreen() ? "on" : "off");
                break;
            case SDLK_UP:
                music_volume += 16;
                if (music_volume > 128) {
                    music_volume = 128;
                }
                mncl_music_volume(music_volume);
                printf("Music volume now: %d\n", music_volume);
                break;
            case SDLK_DOWN:
                music_volume -= 16;
                if (music_volume < 0) {
                    music_volume = 0;
                }
                mncl_music_volume(music_volume);
                printf("Music volume now: %d\n", music_volume);
                break;
            case SDLK_SPACE:
                mncl_play_sfx(sfx, 128);
                break;
            case SDLK_LSHIFT:
                mncl_play_sfx(sfx, 64);
                break;
            default:
                break;
            }
            break;
        case MNCL_EVENT_PRERENDER:
            if (e->value.self) {
                update(e->value.self);
            } else if (countdown > 0) {
                if (--countdown == 0) {
                    done = 1;
                }
            }
            break;
        case MNCL_EVENT_RENDER:
            if (!e->value.self) {
                MNCL_DATA *insts;
                mncl_draw_rect(132, 152, 503, 176, 128, 128, 128);
                insts = mncl_data_resource("instructions");
                if (insts && insts->tag == MNCL_DATA_ARRAY) {
                    int i, y = 156;
                    for (i = 0; i < insts->value.array.size; ++i) {
                        MNCL_DATA *val = insts->value.array.data[i];
                        mncl_draw_string(monospace, 136, y, (val && val->tag == MNCL_DATA_STRING) ? val->value.string : "(invalid datum)");
                        y += 21;
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    mncl_stop_music();
    mncl_uninit();
    return 0;
}
