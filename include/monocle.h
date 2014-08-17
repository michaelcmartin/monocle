#ifndef MONOCLE_H_
#define MONOCLE_H_

#include <stdlib.h>
#include <stdint.h>

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

/* For resources identified as "raw" in the resource map */
extern MONOCULAR MNCL_RAW *mncl_raw_resource(const char *resource);

/* For bypassing the resmap. Leaks if you don't release what you
 * acquire! */
extern MONOCULAR MNCL_RAW *mncl_acquire_raw(const char *resource_name);
extern MONOCULAR void mncl_release_raw(MNCL_RAW *raw);

/* Accessors and decoders */
extern MONOCULAR int mncl_raw_size(MNCL_RAW *raw);
extern MONOCULAR int8_t mncl_raw_s8(MNCL_RAW *raw, int offset);
extern MONOCULAR uint8_t mncl_raw_u8(MNCL_RAW *raw, int offset);
extern MONOCULAR int16_t mncl_raw_s16le(MNCL_RAW *raw, int offset);
extern MONOCULAR int16_t mncl_raw_s16be(MNCL_RAW *raw, int offset);
extern MONOCULAR uint16_t mncl_raw_u16le(MNCL_RAW *raw, int offset);
extern MONOCULAR uint16_t mncl_raw_u16be(MNCL_RAW *raw, int offset);
extern MONOCULAR int32_t mncl_raw_s32le(MNCL_RAW *raw, int offset);
extern MONOCULAR int32_t mncl_raw_s32be(MNCL_RAW *raw, int offset);
extern MONOCULAR uint32_t mncl_raw_u32le(MNCL_RAW *raw, int offset);
extern MONOCULAR uint32_t mncl_raw_u32be(MNCL_RAW *raw, int offset);
extern MONOCULAR int64_t mncl_raw_s64le(MNCL_RAW *raw, int offset);
extern MONOCULAR int64_t mncl_raw_s64be(MNCL_RAW *raw, int offset);
extern MONOCULAR uint64_t mncl_raw_u64le(MNCL_RAW *raw, int offset);
extern MONOCULAR uint64_t mncl_raw_u64be(MNCL_RAW *raw, int offset);
extern MONOCULAR float mncl_raw_f32le(MNCL_RAW *raw, int offset);
extern MONOCULAR float mncl_raw_f32be(MNCL_RAW *raw, int offset);
extern MONOCULAR double mncl_raw_f64le(MNCL_RAW *raw, int offset);
extern MONOCULAR double mncl_raw_f64be(MNCL_RAW *raw, int offset);

/* Key-value map component */

struct struct_MNCL_KV;
typedef struct struct_MNCL_KV MNCL_KV;
typedef void (*MNCL_KV_DELETER)(void *value);
typedef void (*MNCL_KV_VALUE_FN)(const char *key, void *value, void *user);

extern MONOCULAR MNCL_KV *mncl_alloc_kv(MNCL_KV_DELETER deleter);
extern MONOCULAR void mncl_free_kv(MNCL_KV *kv);

extern MONOCULAR int mncl_kv_insert(MNCL_KV *kv, const char *key, void *value);
extern MONOCULAR void *mncl_kv_find(MNCL_KV *kv, const char *key);
extern MONOCULAR void mncl_kv_delete(MNCL_KV *kv, const char *key);

extern MONOCULAR void mncl_kv_foreach(MNCL_KV *kv, MNCL_KV_VALUE_FN fn, void *user);

/* Semi-structured data component */

typedef enum {
    MNCL_DATA_NULL,
    MNCL_DATA_BOOLEAN,
    MNCL_DATA_NUMBER,
    MNCL_DATA_STRING,
    MNCL_DATA_ARRAY,
    MNCL_DATA_OBJECT
} MNCL_DATA_TYPE;

typedef struct mncl_data_value_ {
    MNCL_DATA_TYPE tag;
    union {
        int boolean;
        double number;
        char *string;
        struct { int size; struct mncl_data_value_ **data; } array;
        MNCL_KV *object;
    } value;
} MNCL_DATA;

extern MONOCULAR MNCL_DATA *mncl_data_resource(const char *resource);

extern MONOCULAR MNCL_DATA *mncl_parse_data(const char *data, size_t size);
extern MONOCULAR MNCL_DATA *mncl_data_clone (MNCL_DATA *src);
extern MONOCULAR void mncl_free_data (MNCL_DATA *mncl_data);

extern MONOCULAR const char *mncl_data_error();
extern MONOCULAR MNCL_DATA *mncl_data_lookup(MNCL_DATA *map, const char *key);

/* Framebuffer component */

extern MONOCULAR int mncl_config_video (const char *title, int width, int height, int fullscreen, int flags);
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

extern MONOCULAR MNCL_SPRITESHEET *mncl_spritesheet_resource(const char *resource);
extern MONOCULAR int mncl_spritesheet_width(MNCL_SPRITESHEET *spritesheet);
extern MONOCULAR int mncl_spritesheet_height(MNCL_SPRITESHEET *spritesheet);
extern MONOCULAR void mncl_draw_from_spritesheet(MNCL_SPRITESHEET *spritesheet,
                                int x, int y,
                                int my_x, int my_y,
                                int my_w, int my_h);

/* Music Component */

extern MONOCULAR void mncl_play_music_resource(const char *resource, int fade_in_ms);
extern MONOCULAR void mncl_stop_music(void);
extern MONOCULAR void mncl_fade_out_music(int ms);
extern MONOCULAR void mncl_music_volume(int volume);
extern MONOCULAR void mncl_pause_music(void);
extern MONOCULAR void mncl_resume_music(void);

/* SFX Component */

struct struct_MNCL_SFX;
typedef struct struct_MNCL_SFX MNCL_SFX;

extern MONOCULAR MNCL_SFX *mncl_sfx_resource(const char *resource);
extern MONOCULAR void mncl_play_sfx(MNCL_SFX *sfx, int volume);

/* Sprite component */

struct struct_MNCL_SPRITE;
typedef struct struct_MNCL_SPRITE MNCL_SPRITE;

extern MONOCULAR MNCL_SPRITE *mncl_sprite_resource(const char *resource);
extern MONOCULAR void mncl_draw_sprite(MNCL_SPRITE *s, int x, int y, int frame);

/* Font component */
struct struct_MNCL_FONT;
typedef struct struct_MNCL_FONT MNCL_FONT;

extern MONOCULAR MNCL_FONT *mncl_font_resource(const char *resource);
extern MONOCULAR void mncl_draw_string(MNCL_FONT *font, int x, int y, const char *msg);

/* Object component */

typedef struct struct_MNCL_OBJECT {
    /* location (x, y), animation frame f */
    float x, y, f;
    /* amount these change per frame */
    float dx, dy, df;
    /* Which graphic is used to render or collision check */
    MNCL_SPRITE *sprite;
    /* Information provided by the client to associate with this
     * object. Monocle itself will never touch this data. */
    void *user_data;
    /* There are additional, private members past this in the "true"
     * object class. Only deal with MNCL_OBJECTs via pointers you get
     * from the Monocle API! */
} MNCL_OBJECT;

extern MONOCULAR MNCL_OBJECT *mncl_create_object(float x, float y, const char *kind);
extern MONOCULAR void mncl_object_set_depth(MNCL_OBJECT *object, int depth);
extern MONOCULAR unsigned int mncl_get_trait(const char *trait);

/* Event component */

typedef enum {
    MNCL_EVENT_QUIT,
    MNCL_EVENT_INIT,
    MNCL_EVENT_PREINPUT,
    MNCL_EVENT_KEYDOWN,
    MNCL_EVENT_KEYUP,
    MNCL_EVENT_MOUSEMOVE,
    MNCL_EVENT_MOUSEBUTTONDOWN,
    MNCL_EVENT_MOUSEBUTTONUP,
    MNCL_EVENT_JOYAXISMOVE,
    MNCL_EVENT_JOYBUTTONDOWN,
    MNCL_EVENT_JOYBUTTONUP,
    MNCL_EVENT_JOYHATMOVE,
    MNCL_EVENT_PREPHYSICS,
    MNCL_EVENT_COLLISION,
    MNCL_EVENT_PRERENDER,
    MNCL_EVENT_RENDER,
    MNCL_EVENT_POSTRENDER,
    MNCL_NUM_EVENTS
} MNCL_EVENT_TYPE;

typedef struct struct_MNCL_COLLISION {
    MNCL_OBJECT *self, *other; 
    const char *trait; 
    int trait_id;
} MNCL_COLLISION;

typedef struct struct_MNCL_EVENT {
    MNCL_EVENT_TYPE type;
    union {
        int key;
        struct { int x, y; } mousemove;
        int mousebutton;
        struct { int stick, index, value; } joystick;
        MNCL_OBJECT *self;
        MNCL_COLLISION collision;
    } value;
} MNCL_EVENT;

extern MONOCULAR MNCL_EVENT *mncl_pop_global_event(void);

extern MONOCULAR MNCL_EVENT_TYPE mncl_event_type(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_key(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_mouse_x(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_mouse_y(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_mouse_button(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_joy_stick(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_joy_index(MNCL_EVENT *evt);
extern MONOCULAR int mncl_event_joy_value(MNCL_EVENT *evt);

/* Resource component */
extern MONOCULAR void mncl_load_resmap(const char *path);
extern MONOCULAR void mncl_unload_resmap(const char *path);
extern MONOCULAR void mncl_unload_all_resources(void);

#ifdef __cplusplus
}
#endif

#endif
