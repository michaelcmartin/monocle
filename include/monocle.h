#ifndef MONOCLE_H_
#define MONOCLE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Meta Component */

void mncl_init(void);
void mncl_uninit(void);

/* Raw Data Component */

typedef struct struct_MNCL_RAW {
    unsigned char *data;
    unsigned int size;
} MNCL_RAW;

int mncl_add_resource_directory(const char *pathname);
int mncl_add_resource_zipfile(const char *pathname);
MNCL_RAW *mncl_acquire_raw(const char *resource_name);
void mncl_release_raw(MNCL_RAW *raw);

/* Accessors and decoders */
int mncl_raw_size(MNCL_RAW *raw);
char mncl_raw_s8(MNCL_RAW *raw, int offset);
unsigned char mncl_raw_u8(MNCL_RAW *raw, int offset);
short mncl_raw_s16le(MNCL_RAW *raw, int offset);
short mncl_raw_s16be(MNCL_RAW *raw, int offset);
unsigned short mncl_raw_u16le(MNCL_RAW *raw, int offset);
unsigned short mncl_raw_u16be(MNCL_RAW *raw, int offset);
int mncl_raw_s32le(MNCL_RAW *raw, int offset);
int mncl_raw_s32be(MNCL_RAW *raw, int offset);
unsigned int mncl_raw_u32le(MNCL_RAW *raw, int offset);
unsigned int mncl_raw_u32be(MNCL_RAW *raw, int offset);
long mncl_raw_s64le(MNCL_RAW *raw, int offset);
long mncl_raw_s64be(MNCL_RAW *raw, int offset);
unsigned long mncl_raw_u64le(MNCL_RAW *raw, int offset);
unsigned long mncl_raw_u64be(MNCL_RAW *raw, int offset);
float mncl_raw_f32le(MNCL_RAW *raw, int offset);
float mncl_raw_f32be(MNCL_RAW *raw, int offset);
double mncl_raw_f64le(MNCL_RAW *raw, int offset);
double mncl_raw_f64be(MNCL_RAW *raw, int offset);

/* Framebuffer component */

int mncl_config_video (int width, int height, int fullscreen, int flags);
int mncl_is_fullscreen(void);
int mncl_toggle_fullscreen(void);
void mncl_set_clear_color(unsigned char r, unsigned char g, unsigned char b);

void mncl_begin_frame(void);
void mncl_end_frame(void);

void mncl_draw_rect(int x, int y, int w, int h, 
                    unsigned char r, unsigned char g, unsigned char b, 
                    unsigned char a);

/* Spritesheet Component */

struct struct_MNCL_SPRITESHEET;
typedef struct struct_MNCL_SPRITESHEET MNCL_SPRITESHEET;

MNCL_SPRITESHEET *mncl_alloc_spritesheet(const char *resource_name);
void mncl_free_spritesheet(MNCL_SPRITESHEET *spritesheet);

void mncl_draw_from_spritesheet(MNCL_SPRITESHEET *spritesheet,
                                int x, int y,
                                int my_x, int my_y,
                                int my_w, int my_h);

/* Music Component */

void mncl_play_music(const char *resource_name, int fade_in_ms);
void mncl_stop_music(void);
void mncl_fade_out_music(int ms);
void mncl_music_volume(int volume);
void mncl_pause_music(void);
void mncl_resume_music(void);

/* SFX Component */

struct struct_MNCL_SFX;
typedef struct struct_MNCL_SFX MNCL_SFX;

MNCL_SFX *mncl_alloc_sfx(const char *resource_name);
void mncl_free_sfx(MNCL_SFX *sfx);

void mncl_play_sfx(MNCL_SFX *sfx, int volume);

#ifdef __cplusplus
}
#endif

#endif
