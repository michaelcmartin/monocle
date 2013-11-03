#include <SDL.h>
#include <SDL_mixer.h>
#include "monocle.h"
#include "monocle_internal.h"

void
mncl_init(void)
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf ("SDL_Init Failed! %s\n", SDL_GetError());
    }
    Mix_Init(MIX_INIT_MOD|MIX_INIT_OGG);
    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 1, 1024)) {
        printf("Could not open audio: %s\n", Mix_GetError());
    }
    Mix_AllocateChannels(4);
    Mix_Volume(-1, 128);    
}

void
mncl_uninit()
{
    mncl_unload_all_resources();
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
    mncl_uninit_raw_system();
}
