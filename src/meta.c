#include <SDL.h>
#include <SDL_mixer.h>
#include "monocle.h"
#include "monocle_internal.h"

void
mncl_init(void)
{
    if (SDL_Init(0)) {
        printf ("OMGWTFBBQ SDL_Init Failed! %s\n", SDL_GetError());
    }
    if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        printf ("OMGWTFBBQ SDL_Init Video Failed! %s\n", SDL_GetError());
    }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        printf ("OMGWTFBBQ SDL_Init Audio Failed! %s\n", SDL_GetError());
    }
    if (SDL_InitSubSystem(SDL_INIT_TIMER)) {
        printf ("OMGWTFBBQ SDL_Init Timer Failed! %s\n", SDL_GetError());
    }
    Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 1, 1024);    
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
