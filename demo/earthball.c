#include <stdlib.h>
#include <SDL.h>
#include "monocle.h"

struct globe {
    int x, y, dx, dy, frame;
};

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

int
main(int argc, char **argv)
{
    MNCL_SPRITESHEET *earth;
    int done, frame, i, countdown, music_on, bg, music_volume;
    Uint32 targetTime;
    struct globe globes[16];

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
    mncl_play_music("march.it", 2000);

    done = 0;
    frame = 0;
    countdown = 0;
    music_on = 1;
    bg = 0;
    music_volume = 128;

    targetTime = SDL_GetTicks() + 40; // Target FPS is 25

    instructions();
    while (!done) {
        SDL_Event e;

        mncl_begin_frame();
        for (i = 0; i < 16; ++i) {
            mncl_draw_from_spritesheet(earth, globes[i].x, globes[i].y, (globes[i].frame % 8) * 64, (globes[i].frame / 8) * 64, 64, 64);
        }
        mncl_end_frame();

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

        while (SDL_PollEvent(&e)) {
            switch(e.type) {
            case SDL_QUIT:
                done = 1;
                break;
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
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
                    printf("Toggled fullscreen to: %s", mncl_toggle_fullscreen() ? "on" : "off");
                    printf("Fullscreen reported as: %s", mncl_is_fullscreen() ? "on" : "off");
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
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime < targetTime) {
            Uint32 waitTime = targetTime - currentTime;
            if (waitTime > 0 && waitTime < 1000) {
                SDL_Delay(waitTime);
            }
        }
        targetTime = SDL_GetTicks() + 40;
        if (countdown > 0) {
            if (--countdown == 0) {
                done = 1;
            }
        }
    }

    mncl_stop_music();
    mncl_free_spritesheet(earth);
    mncl_uninit();
    return 0;
}
