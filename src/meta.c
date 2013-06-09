#include <SDL.h>
#include <SDL_mixer.h>
#include "monocle.h"

void
mncl_init(void)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 1, 1024);    
    Mix_AllocateChannels(4);
    Mix_Volume(-1, 128);    
}

void
mncl_uninit()
{
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
}
