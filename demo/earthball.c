#include <stdlib.h>
#include <SDL.h>
#include "monocle.h"

struct globe {
    int x, y, dx, dy, frame;
};

struct globe globes[16];
MNCL_SPRITESHEET *earth;

void
instructions(void)
{
    printf("Instructions:\n");
    printf("    B to change background color\n");
    printf("    P to pause/resume music\n");
    printf("    T to toggle and test fullscreen\n");
    printf("    SPACE to play loud sound effect\n");
    printf("    LEFT SHIFT to play soft sound effect\n");
    printf("    UP AND DOWN ARROW KEYS to change music volume\n");
    printf("    ESCAPE to quit with music fadeout\n");
    printf("    Close the window to quit immediately\n");
}

void
update(void)
{
    int i;
    for (i = 0; i < 16; ++i) {
        globes[i].x += globes[i].dx;
        globes[i].y += globes[i].dy;
        if (globes[i].x < 0) {
            globes[i].x = 0; globes[i].dx = -globes[i].dx;
        }
        if (globes[i].y < 0) {
            globes[i].y = 0; globes[i].dy = -globes[i].dy;
        }
        if (globes[i].x > 704) {
            globes[i].x = 704; globes[i].dx = -globes[i].dx;
        }
        if (globes[i].y > 416) {
            globes[i].y = 416; globes[i].dy = -globes[i].dy;
        }
        globes[i].frame = (globes[i].frame + 1) % 30;
    }
}

void
render(void)
{
    int i;
    mncl_draw_rect(256, 112, 256, 256, 128, 128, 128);
    for (i = 0; i < 16; ++i) {
        mncl_draw_from_spritesheet(earth, globes[i].x, globes[i].y, (globes[i].frame % 8) * 64, (globes[i].frame / 8) * 64, 64, 64);
    }
}

int
main(int argc, char **argv)
{
    MNCL_SFX *sfx;
    int done, i, countdown, music_on, bg, music_volume;

    for (i = 0; i < 16; ++i) {
        globes[i].x = rand() % (768 - 64);
        globes[i].y = rand() % (480 - 64);
        globes[i].dx = (rand() % 5 + 1) * ((rand() % 2) ? 1 : -1);
        globes[i].dy = (rand() % 5 + 1) * ((rand() % 2) ? 1 : -1);
        globes[i].frame = rand() % 30;
    }

    mncl_init();
    mncl_config_video(768, 480, 0, 0);
    mncl_add_resource_zipfile("earthball-res.zip");

    earth = mncl_alloc_spritesheet("earth.png");
    sfx = mncl_alloc_sfx("torpedo.wav");
    mncl_play_music_file("march.it", 2000);

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
        case MNCL_EVENT_UPDATE:
            update();
            if (countdown > 0) {
                if (--countdown == 0) {
                    done = 1;
                }
            }
            break;
        case MNCL_EVENT_RENDER:
            render();
            break;
        default:
            break;
        }
    }

    mncl_stop_music();
    mncl_free_sfx(sfx);
    mncl_free_spritesheet(earth);
    mncl_uninit();
    return 0;
}
