#include <stdlib.h>
#include <SDL.h>
#include "monocle.h"

struct globe {
    int x, y, dx, dy, frame;
};

int
main(int argc, char **argv)
{
    MNCL_SPRITESHEET *earth;
    int done, frame, i;
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
    targetTime = SDL_GetTicks() + 40; // Target FPS is 25
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
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    done = 1;
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
    }

    mncl_stop_music();
    mncl_free_spritesheet(earth);
    mncl_uninit();
    return 0;
}
