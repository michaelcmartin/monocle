#ifndef MONOCLE_H_
#define MONOCLE_H_

#ifndef MONOCULAR
#ifdef MONOCLE_EXPORTS
#ifdef _WIN32
#define MONOCULAR __declspec(dllexport)
#else
#define MONOCULAR __attribute__ ((visibility("default")))
#endif
#else
#ifdef _WIN32
#define MONOCULAR __declspec(dllimport)
#else
#define MONOCULAR
#endif
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Meta Component */

extern MONOCULAR void mncl_init(void);
extern MONOCULAR void mncl_uninit(void);

/* Raw Data Component */

typedef struct struct_MNCL_RAW {
    unsigned char *data;
    unsigned int size;
} MNCL_RAW;

extern MONOCULAR int mncl_add_resource_directory(const char *pathname);
extern MONOCULAR int mncl_add_resource_zipfile(const char *pathname);
extern MONOCULAR MNCL_RAW *mncl_acquire_raw(const char *resource_name);
extern MONOCULAR void mncl_release_raw(MNCL_RAW *raw);

/* Accessors and decoders */
extern MONOCULAR int mncl_raw_size(MNCL_RAW *raw);
extern MONOCULAR char mncl_raw_s8(MNCL_RAW *raw, int offset);
extern MONOCULAR unsigned char mncl_raw_u8(MNCL_RAW *raw, int offset);
extern MONOCULAR short mncl_raw_s16le(MNCL_RAW *raw, int offset);
extern MONOCULAR short mncl_raw_s16be(MNCL_RAW *raw, int offset);
extern MONOCULAR unsigned short mncl_raw_u16le(MNCL_RAW *raw, int offset);
extern MONOCULAR unsigned short mncl_raw_u16be(MNCL_RAW *raw, int offset);
extern MONOCULAR int mncl_raw_s32le(MNCL_RAW *raw, int offset);
extern MONOCULAR int mncl_raw_s32be(MNCL_RAW *raw, int offset);
extern MONOCULAR unsigned int mncl_raw_u32le(MNCL_RAW *raw, int offset);
extern MONOCULAR unsigned int mncl_raw_u32be(MNCL_RAW *raw, int offset);
extern MONOCULAR long mncl_raw_s64le(MNCL_RAW *raw, int offset);
extern MONOCULAR long mncl_raw_s64be(MNCL_RAW *raw, int offset);
extern MONOCULAR unsigned long mncl_raw_u64le(MNCL_RAW *raw, int offset);
extern MONOCULAR unsigned long mncl_raw_u64be(MNCL_RAW *raw, int offset);
extern MONOCULAR float mncl_raw_f32le(MNCL_RAW *raw, int offset);
extern MONOCULAR float mncl_raw_f32be(MNCL_RAW *raw, int offset);
extern MONOCULAR double mncl_raw_f64le(MNCL_RAW *raw, int offset);
extern MONOCULAR double mncl_raw_f64be(MNCL_RAW *raw, int offset);

/* Framebuffer component */

extern MONOCULAR int mncl_config_video (int width, int height, int fullscreen, int flags);
extern MONOCULAR int mncl_is_fullscreen(void);
extern MONOCULAR int mncl_toggle_fullscreen(void);
extern MONOCULAR void mncl_set_clear_color(unsigned char r, unsigned char g, unsigned char b);
extern MONOCULAR void mncl_hide_mouse_in_fullscreen(int val);

extern MONOCULAR void mncl_begin_frame(void);
extern MONOCULAR void mncl_end_frame(void);

extern MONOCULAR void mncl_draw_rect(int x, int y, int w, int h, 
                                     unsigned char r, unsigned char g, unsigned char b);

/* Spritesheet Component */

struct struct_MNCL_SPRITESHEET;
typedef struct struct_MNCL_SPRITESHEET MNCL_SPRITESHEET;

extern MONOCULAR MNCL_SPRITESHEET *mncl_alloc_spritesheet(const char *resource_name);
extern MONOCULAR void mncl_free_spritesheet(MNCL_SPRITESHEET *spritesheet);

extern MONOCULAR void mncl_normalize_spritesheet(MNCL_SPRITESHEET *spritesheet);

extern MONOCULAR void mncl_draw_from_spritesheet(MNCL_SPRITESHEET *spritesheet,
                                int x, int y,
                                int my_x, int my_y,
                                int my_w, int my_h);

/* Music Component */

extern MONOCULAR void mncl_play_music_file(const char *pathname, int fade_in_ms);
extern MONOCULAR void mncl_stop_music(void);
extern MONOCULAR void mncl_fade_out_music(int ms);
extern MONOCULAR void mncl_music_volume(int volume);
extern MONOCULAR void mncl_pause_music(void);
extern MONOCULAR void mncl_resume_music(void);

/* SFX Component */

struct struct_MNCL_SFX;
typedef struct struct_MNCL_SFX MNCL_SFX;

extern MONOCULAR MNCL_SFX *mncl_alloc_sfx(const char *resource_name);
extern MONOCULAR void mncl_free_sfx(MNCL_SFX *sfx);

extern MONOCULAR void mncl_play_sfx(MNCL_SFX *sfx, int volume);

/* Event component */

typedef enum {
    MNCL_EVENT_QUIT,
    MNCL_EVENT_PREUPDATE,
    MNCL_EVENT_KEYDOWN,
    MNCL_EVENT_KEYUP,
    MNCL_EVENT_MOUSEMOVE,
    MNCL_EVENT_MOUSEBUTTONDOWN,
    MNCL_EVENT_MOUSEBUTTONUP,
    MNCL_EVENT_JOYAXISMOVE,
    MNCL_EVENT_JOYBUTTONDOWN,
    MNCL_EVENT_JOYBUTTONUP,
    MNCL_EVENT_JOYHATMOVE,
    MNCL_EVENT_UPDATE,
    MNCL_EVENT_COLLISION,
    MNCL_EVENT_POSTUPDATE,
    MNCL_EVENT_PRERENDER,
    MNCL_EVENT_RENDER,
    MNCL_EVENT_POSTRENDER,
    MNCL_EVENT_FRAMEBOUNDARY,
    /* ... */
    MNCL_NUM_EVENTS
} MNCL_EVENT_TYPE;

typedef struct struct_MNCL_EVENT {
    MNCL_EVENT_TYPE type;
    int subscriber;
    union {
        int key;
        struct { int x, y; } mousemove;
        int mousebutton;
        struct { int stick, index, value; } joystick;
        /* ... */
    } value;
} MNCL_EVENT;

extern MONOCULAR MNCL_EVENT *mncl_pop_global_event(void);

extern MONOCULAR MNCL_EVENT_TYPE mncl_event_type(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_subscriber(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_key(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_mouse_x(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_mouse_y(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_mouse_button(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_joy_stick(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_joy_index(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_joy_value(MNCL_EVENT *evt);

/* Sprite component. Everything but mncl_draw_sprite may eventually be
 * internalized. */

typedef struct struct_MNCL_FRAME {
    MNCL_SPRITESHEET *sheet;
    int x, y;
} MNCL_FRAME;

typedef struct struct_MNCL_SPRITE {
    int w, h, hot_x, hot_y, hit_x, hit_y, hit_w, hit_h;
    int nframes;
    MNCL_FRAME frames[0];
} MNCL_SPRITE;

extern MONOCULAR MNCL_SPRITE *mncl_alloc_sprite(int nframes);
extern MONOCULAR void mncl_free_sprite(MNCL_SPRITE *sprite);
extern MONOCULAR void mncl_draw_sprite(MNCL_SPRITE *s, int x, int y, int frame);

#ifdef __cplusplus
}
#endif

#endif
