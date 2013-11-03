#include <SDL.h>
#include <SDL_mixer.h>
#include "string.h"
#include "monocle.h"

/* We can save ourselves some grief if it's legal to release the raw
 * data after the music is constructed; however, I'm pretty sure this
 * doesn't work */

static Mix_Music *current_bgm = NULL;
static char *current_bgm_name = NULL;
static MNCL_RAW *current_bgm_raw = NULL;

/* We can save ourselves some grief if it's legal to release the raw
 * data after the chunk is constructed. For SFX I have no idea if this
 * works */
struct struct_MNCL_SFX {
    MNCL_RAW *raw;
    Mix_Chunk *chunk;
};

void
mncl_play_music_file(const char *pathname, int fade_in_ms)
{
    SDL_RWops *bgm_rw;
    int name_size = strlen(pathname) + 1;
    if (current_bgm_name && !strcmp(pathname, current_bgm_name)) {
        /* We're already playing that music */
        return;
    }
    mncl_stop_music();
    current_bgm_raw = mncl_acquire_raw(pathname);
    if (!current_bgm_raw) {
        /* TODO: Error code? */
        return;
    }
    current_bgm_name = (char *)malloc(name_size);
    if (!current_bgm_name) {
        /* Life is pain if this happens */
        mncl_stop_music();
        return;
    }
    strncpy(current_bgm_name, pathname, name_size);

    bgm_rw = SDL_RWFromMem(current_bgm_raw->data, current_bgm_raw->size);
    if (!bgm_rw) {
        mncl_stop_music();
        return;
    }
    current_bgm = Mix_LoadMUS_RW(bgm_rw, 1);
    if (!current_bgm) {
        /* Should probably log some kind of error here */
        mncl_stop_music();
        return;
    }
    if (fade_in_ms > 0) {
        Mix_FadeInMusic(current_bgm, -1, fade_in_ms);
    } else {
        Mix_PlayMusic(current_bgm, -1);
    }
}

void
mncl_stop_music(void)
{
    Mix_ExpireChannel(-1, 0);
    if (current_bgm) {
        Mix_HaltMusic();
        Mix_FreeMusic(current_bgm);
        current_bgm = NULL;
    }
    if (current_bgm_name) {
        free(current_bgm_name);
        current_bgm_name = NULL;
    }
    if (current_bgm_raw) {
        mncl_release_raw(current_bgm_raw);
        current_bgm_raw = NULL;
    }
}

void
mncl_fade_out_music(int ms)
{
    Mix_FadeOutMusic(ms);
}

void
mncl_music_volume(int volume)
{
    Mix_VolumeMusic(volume);
}

void
mncl_pause_music(void)
{
    Mix_PauseMusic();
}

void
mncl_resume_music(void)
{
    Mix_ResumeMusic();
}

MNCL_SFX *
mncl_alloc_sfx(const char *resource_name)
{
    MNCL_RAW *raw;
    SDL_RWops *rwops;
    Mix_Chunk *chunk;
    MNCL_SFX *result;
    

    raw = mncl_acquire_raw(resource_name);
    if (!raw) {
        return NULL;
    }

    {
        int audio_rate;
        Uint16 audio_format;
        int audio_channels;
        Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
        printf("Opened audio at %d Hz %d bit %s", audio_rate,
            (audio_format&0xFF),
            (audio_channels > 2) ? "surround" :
            (audio_channels > 1) ? "stereo" : "mono");
    }

    rwops = SDL_RWFromMem(raw->data, raw->size);
    if (!rwops) {
        mncl_release_raw(raw);
        return NULL;
    }

    chunk = Mix_LoadWAV_RW(rwops, 1);
    if (!chunk) {
        mncl_release_raw(raw);
        return NULL;
    }

    result = malloc(sizeof(MNCL_SFX));
    if (!result) {
        Mix_FreeChunk(chunk);
        mncl_release_raw(raw);
        return NULL;
    }
    result->raw = raw;
    result->chunk = chunk;
    return result;
}

void
mncl_free_sfx(MNCL_SFX *sfx)
{
    if (!sfx) {
        return;
    }
    Mix_FreeChunk(sfx->chunk);
    mncl_release_raw(sfx->raw);
    free(sfx);
}


void
mncl_play_sfx(MNCL_SFX *sfx, int volume)
{
    int channel;
    if (!sfx) {
        return;
    }
    channel = Mix_PlayChannel(-1, sfx->chunk, 0);
    Mix_Volume(channel, volume);
}
