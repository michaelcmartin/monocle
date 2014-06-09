#ifndef MONOCLE_INTERNAL_H_
#define MONOCLE_INTERNAL_H_

#include "monocle.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Raw */
void mncl_uninit_raw_system(void);

/* Spritesheets */
MNCL_SPRITESHEET *mncl_alloc_spritesheet(const char *resource_name);
void mncl_free_spritesheet(MNCL_SPRITESHEET *spritesheet);
void mncl_normalize_spritesheet(MNCL_SPRITESHEET *spritesheet);
void mncl_renormalize_all_spritesheets(void);

/* Sprites */
typedef struct struct_MNCL_FRAME {
    MNCL_SPRITESHEET *sheet;
    int x, y;
} MNCL_FRAME;

struct struct_MNCL_SPRITE {
    int w, h, hot_x, hot_y, hit_x, hit_y, hit_w, hit_h;
    int nframes;
    MNCL_FRAME frames[0];
};

MNCL_SPRITE *mncl_alloc_sprite(int nframes);
void mncl_free_sprite(MNCL_SPRITE *sprite);

/* Fonts */
typedef struct struct_MNCL_FONT {
    MNCL_SPRITESHEET *spritesheet;
    int w, h, tile_w, tile_h, hot_x, hot_y, first, last;
} MNCL_FONT;

/* SFX */
MNCL_SFX *mncl_alloc_sfx(const char *resource_name);
void mncl_free_sfx(MNCL_SFX *sfx);

/* Music */
void mncl_play_music_file(const char *pathname, int fade_in_ms);

/* Kinds and Traits */
typedef struct struct_MNCL_KIND {
    float f, dx, dy, df, depth;
    int visible, customrender;
    MNCL_SPRITE *sprite;
    unsigned int *traits, *collisions;
} MNCL_KIND;

MNCL_KIND *mncl_kind_resource(const char *resource);
unsigned int mncl_get_trait(const char *trait);
void mncl_uninit_traits(void);

/* Objects */
void initialize_object_trees(void);
MNCL_OBJECT *object_begin(MNCL_EVENT_TYPE which);
MNCL_OBJECT *object_next(void);
void collision_begin(MNCL_COLLISION *collision);
void collision_next(MNCL_COLLISION *collision);
void default_update_all_objects(void);
void default_render_all_objects(void);

#ifdef __cplusplus
}
#endif

#endif
