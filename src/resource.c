#include <stdio.h>
#include <string.h>
#include "monocle.h"
#include "monocle_internal.h"
#include "tree.h"

typedef void *(*ALLOC_FN)(MNCL_DATA *);

typedef struct res_class {
    MNCL_KV values;
    const char *type;
    ALLOC_FN alloc_fn;
} RES_CLASS;

static void *
raw_alloc(MNCL_DATA *arg)
{
    if (!arg || arg->tag != MNCL_DATA_STRING) {
        return NULL;
    }
    return mncl_acquire_raw(arg->value.string);
}

static void *
spritesheet_alloc(MNCL_DATA *arg)
{
    if (arg && arg->tag == MNCL_DATA_STRING) {
        return mncl_alloc_spritesheet(arg->value.string);
    }
    return NULL;
}

static void *
sprite_alloc(MNCL_DATA *arg)
{
    if (arg && arg->tag == MNCL_DATA_OBJECT) {
        MNCL_DATA *mncl_data_w = mncl_data_lookup(arg, "width");
        MNCL_DATA *mncl_data_h = mncl_data_lookup(arg, "height");
        MNCL_DATA *mncl_data_fr = mncl_data_lookup(arg, "frames");
        MNCL_DATA *hot_x = mncl_data_lookup(arg, "hotspot-x");
        MNCL_DATA *hot_y = mncl_data_lookup(arg, "hotspot-y");
        MNCL_DATA *hit_x = mncl_data_lookup(arg, "hitbox-x");
        MNCL_DATA *hit_y = mncl_data_lookup(arg, "hitbox-y");
        MNCL_DATA *hit_w = mncl_data_lookup(arg, "hitbox-width");
        MNCL_DATA *hit_h = mncl_data_lookup(arg, "hitbox-height");

        MNCL_SPRITE *result = NULL;
        int i;
        if (!(mncl_data_w && mncl_data_h && mncl_data_fr)) {
            return NULL;
        }
        if (mncl_data_w->tag != MNCL_DATA_NUMBER || mncl_data_h->tag != MNCL_DATA_NUMBER) {
            return NULL;
        }
        if (mncl_data_fr->tag != MNCL_DATA_ARRAY) {
            return NULL;
        }
        result = mncl_alloc_sprite(mncl_data_fr->value.array.size);
        if (!result) {
            return NULL;
        }
        result->w = (int)mncl_data_w->value.number;
        result->h = (int)mncl_data_h->value.number;
        /* Some sensible defaults for optional values */
        result->hot_x = result->hot_y = 0;
        result->hit.shape = MNCL_HITBOX_BOX;
        result->hit.hit.box.x = result->hit.hit.box.y = 0;
        result->hit.hit.box.w = result->w;
        result->hit.hit.box.h = result->h;
        /* Then override those if they were in the object */
        if (hot_x && hot_x->tag == MNCL_DATA_NUMBER) {
            result->hot_x = (int)hot_x->value.number;
        }
        if (hot_y && hot_y->tag == MNCL_DATA_NUMBER) {
            result->hot_y = (int)hot_y->value.number;
        }
        if (hit_x && hit_x->tag == MNCL_DATA_NUMBER) {
            result->hit.hit.box.x = (int)hit_x->value.number;
        }
        if (hit_y && hit_y->tag == MNCL_DATA_NUMBER) {
            result->hit.hit.box.y = (int)hit_y->value.number;
        }
        if (hit_w && hit_w->tag == MNCL_DATA_NUMBER) {
            result->hit.hit.box.w = (int)hit_w->value.number;
        }
        if (hit_h && hit_h->tag == MNCL_DATA_NUMBER) {
            result->hit.hit.box.h = (int)hit_h->value.number;
        }
        for (i = 0; i < result->nframes; ++i) {
            MNCL_DATA *mncl_data_f = mncl_data_fr->value.array.data[i];
            if (mncl_data_f && mncl_data_f->tag == MNCL_DATA_OBJECT) {
                MNCL_DATA *x = mncl_data_lookup(mncl_data_f, "x");
                MNCL_DATA *y = mncl_data_lookup(mncl_data_f, "y");
                MNCL_DATA *ss = mncl_data_lookup(mncl_data_f, "spritesheet");
                if (x && y && ss && x->tag == MNCL_DATA_NUMBER && y->tag == MNCL_DATA_NUMBER && ss->tag == MNCL_DATA_STRING) {
                    MNCL_SPRITESHEET *ss_val = mncl_spritesheet_resource(ss->value.string);
                    if (ss_val) {
                        result->frames[i].sheet = ss_val;
                        result->frames[i].x = (int)x->value.number;
                        result->frames[i].y = (int)y->value.number;
                        /* Everything worked! */
                        continue;
                    }
                }
            }
            /* If we get here, something went wrong, so fail out. */
            mncl_free_sprite(result);
            return NULL;
        }
        return result;
    }
    return NULL;
}

static void *
font_alloc(MNCL_DATA *arg)
{
    if (arg && arg->tag == MNCL_DATA_OBJECT) {
        MNCL_DATA *mncl_data_w = mncl_data_lookup(arg, "width");
        MNCL_DATA *mncl_data_h = mncl_data_lookup(arg, "height");
        MNCL_DATA *mncl_data_first = mncl_data_lookup(arg, "first-index");
        MNCL_DATA *mncl_data_last = mncl_data_lookup(arg, "last-index");
        MNCL_DATA *mncl_data_ss = mncl_data_lookup(arg, "spritesheet");
        MNCL_DATA *tile_w = mncl_data_lookup(arg, "tile-width");
        MNCL_DATA *tile_h = mncl_data_lookup(arg, "tile-height");
        MNCL_DATA *hot_x = mncl_data_lookup(arg, "hotspot-x");
        MNCL_DATA *hot_y = mncl_data_lookup(arg, "hotspot-y");

        MNCL_FONT *result = NULL;
        MNCL_SPRITESHEET *spritesheet = NULL;
        if (!(mncl_data_w && mncl_data_h && mncl_data_first && mncl_data_last && mncl_data_ss)) {
            return NULL;
        }
        if (mncl_data_w->tag != MNCL_DATA_NUMBER || mncl_data_h->tag != MNCL_DATA_NUMBER) {
            return NULL;
        }
        if (mncl_data_first->tag != MNCL_DATA_NUMBER || mncl_data_last->tag != MNCL_DATA_NUMBER) {
            return NULL;
        }
        if (mncl_data_ss->tag != MNCL_DATA_STRING) {
            return NULL;
        }
        if (mncl_data_w->value.number == 0) {
            return NULL;
        }
        spritesheet = mncl_spritesheet_resource(mncl_data_ss->value.string);
        if (!spritesheet) {
            return NULL;
        }

        result = (MNCL_FONT *)malloc(sizeof(MNCL_FONT));
        if (!result) {
            return NULL;
        }
        result->w = (int)mncl_data_w->value.number;
        result->h = (int)mncl_data_h->value.number;
        result->first = (int)mncl_data_first->value.number;
        result->last = (int)mncl_data_last->value.number;
        result->spritesheet = spritesheet;
        /* Some sensible defaults for optional values */
        result->hot_x = result->hot_y = 0;
        result->tile_w = result->w;
        result->tile_h = result->h;
        /* Then override those if they were in the object */
        if (hot_x && hot_x->tag == MNCL_DATA_NUMBER) {
            result->hot_x = (int)hot_x->value.number;
        }
        if (hot_y && hot_y->tag == MNCL_DATA_NUMBER) {
            result->hot_y = (int)hot_y->value.number;
        }
        if (tile_w && tile_w->tag == MNCL_DATA_NUMBER) {
            result->tile_w = (int)tile_w->value.number;
            if (!result->tile_w) {
                result->tile_w = result->w;
            }
        }
        if (tile_h && tile_h->tag == MNCL_DATA_NUMBER) {
            result->tile_h = (int)tile_h->value.number;
        }
        result->chars_per_row = mncl_spritesheet_width(result->spritesheet) / result->tile_w;
        return result;
    }
    return NULL;
}

static void *
sfx_alloc(MNCL_DATA *arg)
{
    if (arg && arg->tag == MNCL_DATA_STRING) {
        return mncl_alloc_sfx(arg->value.string);
    }
    return NULL;
}

static void *
music_alloc(MNCL_DATA *arg)
{
    if (arg && arg->tag == MNCL_DATA_STRING) {
        int size = strlen(arg->value.string) + 1;
        char *result = malloc(size);
        strncpy(result, arg->value.string, size);
        return result;
    }
    return NULL;
}

static void *
data_alloc(MNCL_DATA *arg)
{
    return mncl_data_clone(arg);
}

static void *
kind_alloc(MNCL_DATA *arg)
{
    MNCL_KIND *result = NULL;
    int trait_count;
    unsigned int invisible, customrender;
    if (arg && arg->tag == MNCL_DATA_OBJECT) {
        result = malloc(sizeof(MNCL_KIND));
        if (result) {
            MNCL_DATA *v;
            /* Set some defaults */
            result->dx = 0;
            result->dy = 0;
            result->f = 0;
            result->df = 1;
            result->sprite = NULL;
            result->visible = 1;
            result->customrender = 0;
            /* Fill in the optional overrides */
            v = mncl_data_lookup(arg, "dx");
            if (v && v->tag == MNCL_DATA_NUMBER) {
                result->dx = (float)v->value.number;
            }
            v = mncl_data_lookup(arg, "dy");
            if (v && v->tag == MNCL_DATA_NUMBER) {
                result->dy = (float)v->value.number;
            }
            v = mncl_data_lookup(arg, "frame");
            if (v && v->tag == MNCL_DATA_NUMBER) {
                result->f = (float)v->value.number;
            }
            v = mncl_data_lookup(arg, "frame-speed");
            if (v && v->tag == MNCL_DATA_NUMBER) {
                result->df = (float)v->value.number;
            }
            v = mncl_data_lookup(arg, "depth");
            if (v && v->tag == MNCL_DATA_NUMBER) {
                result->depth = (float)v->value.number;
            }
            v = mncl_data_lookup(arg, "sprite");
            if (v && v->tag == MNCL_DATA_STRING) {
                MNCL_SPRITE *s = mncl_sprite_resource(v->value.string);
                if (!s) {
                    printf("WARNING: Kind specifies unknown sprite '%s'\n", v->value.string);
                }
                result->sprite = s;
            }
            /* Now for the fun part: lists of traits */
            trait_count = 1; /* Start with just the terminator */
            invisible = mncl_get_trait("invisible");
            customrender = mncl_get_trait("render");
            v = mncl_data_lookup(arg, "traits");
            if (v && v->tag == MNCL_DATA_ARRAY) {
                int i;
                for (i = 0; i < v->value.array.size; ++i) {
                    MNCL_DATA *t = v->value.array.data[i];
                    if (t && t->tag == MNCL_DATA_STRING) {
                        unsigned int t_id = mncl_get_trait(t->value.string);
                        if (t_id != invisible && t_id != customrender) {
                            ++trait_count;
                        }
                    }
                }
            }
            result->traits = (unsigned int *)malloc(sizeof(unsigned int) * trait_count);
            if (result->traits) {
                int i;
                trait_count = 0;
                if (v && v->tag == MNCL_DATA_ARRAY) {
                    for (i = 0; i < v->value.array.size; ++i) {
                        MNCL_DATA *t = v->value.array.data[i];
                        if (t && t->tag == MNCL_DATA_STRING) {
                            unsigned int t_id = mncl_get_trait(t->value.string);
                            if (t_id == invisible) {
                                result->visible = 0;
                            } else if (t_id == customrender) {
                                result->customrender = 1;
                            } else {
                                result->traits[trait_count++] = t_id;
                            }
                        } else {
                            printf("WARNING: Traits need to be strings");
                        }
                    }
                }
                result->traits[trait_count] = 0; /* Set the terminator */
            } else {
                printf("ERROR: Could not allocate the trait array!\n");
                free (result);
                return NULL;
            }
            trait_count = 1; /* Start with just the terminator */
            v = mncl_data_lookup(arg, "collisions");
            if (v && v->tag == MNCL_DATA_ARRAY) {
                int i;
                for (i = 0; i < v->value.array.size; ++i) {
                    MNCL_DATA *t = v->value.array.data[i];
                    if (t && t->tag == MNCL_DATA_STRING) {
                        ++trait_count;
                    }
                }
            }
            result->collisions = (unsigned int *)malloc(sizeof(unsigned int) * trait_count);
            if (result->collisions) {
                int i;
                trait_count = 0;
                if (v && v->tag == MNCL_DATA_ARRAY) {
                    for (i = 0; i < v->value.array.size; ++i) {
                        MNCL_DATA *t = v->value.array.data[i];
                        if (t && t->tag == MNCL_DATA_STRING) {
                            result->collisions[trait_count++] = mncl_get_trait(t->value.string);
                        } else {
                            printf("WARNING: Collisions need to be strings");
                        }
                    }
                }
                result->collisions[trait_count] = 0; /* Set the terminator */
            } else {
                printf("ERROR: Could not allocate the collisions array!\n");
                free (result->traits);
                free (result);
                return NULL;
            }
        }
    }
    return result;
}

static void
mncl_free_kind(void *obj) {
    MNCL_KIND *kind = (MNCL_KIND *)obj;
    if (kind) {
        if (kind->traits) {
            free(kind->traits);
        }
        if (kind->collisions) {
            free(kind->collisions);
        }
        free(kind);
    }
}

static RES_CLASS raw = { { { NULL }, (MNCL_KV_DELETER)mncl_release_raw }, "raw", raw_alloc };
static RES_CLASS spritesheet = { { { NULL }, (MNCL_KV_DELETER)mncl_free_spritesheet }, "spritesheet", spritesheet_alloc };
static RES_CLASS sprite = { { { NULL }, (MNCL_KV_DELETER)mncl_free_sprite },  "sprite", sprite_alloc };
static RES_CLASS font = { { { NULL }, free }, "font", font_alloc };
static RES_CLASS sfx = { { { NULL }, (MNCL_KV_DELETER)mncl_free_sfx }, "sfx", sfx_alloc };
static RES_CLASS music = { { { NULL }, free }, "music", music_alloc };
static RES_CLASS data = { { { NULL }, (MNCL_KV_DELETER)mncl_free_data }, "data", data_alloc };
static RES_CLASS kind = { { { NULL }, (MNCL_KV_DELETER)mncl_free_kind }, "kind", kind_alloc };

static RES_CLASS *resclasses[] = { &raw, &spritesheet, &sprite, &font, &sfx, &music, &data, &kind, NULL };

static void
alloc_resource_type(const char *key, void *value, void *user)
{
    RES_CLASS *rc = (RES_CLASS *)user;
    void *val = rc->alloc_fn((MNCL_DATA *)value);
    if (val) {
        void *old_val = mncl_kv_find(&rc->values, key);
        if (old_val) {
            printf("WARNING: overwriting %s resource %s\n", rc->type, key);
        }
        if (!mncl_kv_insert(&rc->values, key, val)) {
            printf("WARNING: Could not store %s resource %s\n", rc->type, key);
            rc->values.deleter(val);
        }
    } else {
        printf ("WARNING: Could not handle %s resource %s\n", rc->type, key);
    }
}

static void
free_resource_type (const char *key, void *value, void *user)
{
    RES_CLASS *rc = (RES_CLASS *)user;
    mncl_kv_delete(&rc->values, key);
}

void
mncl_load_resmap(const char *path)
{
    MNCL_DATA *resmap = NULL;
    MNCL_RAW *resmap_file = mncl_acquire_raw(path);
    if (!resmap_file) {
        printf ("WARNING: Could not find resource map %s\n", path);
        return;
    }
    resmap = mncl_parse_data((char *)resmap_file->data, resmap_file->size);
    mncl_release_raw(resmap_file);
    if (resmap) {
        int i;
        for (i = 0; resclasses[i]; ++i) {
            MNCL_DATA *top = mncl_data_lookup(resmap, resclasses[i]->type);
            if (top && top->tag == MNCL_DATA_OBJECT) {
                mncl_kv_foreach(top->value.object, alloc_resource_type, resclasses[i]);
            }
        }
        mncl_free_data(resmap);
    }
}

/* Unloaders */

void
mncl_unload_resmap(const char *path)
{
    MNCL_DATA *resmap = NULL;
    MNCL_RAW *resmap_file = mncl_acquire_raw(path);
    if (!resmap_file) {
        return;
    }
    resmap = mncl_parse_data((char *)resmap_file->data, resmap_file->size);
    mncl_release_raw(resmap_file);
    if (resmap) {
        int i;
        for (i = 0; resclasses[i]; ++i) {
            MNCL_DATA *top = mncl_data_lookup(resmap, resclasses[i]->type);
            if (top && top->tag == MNCL_DATA_OBJECT) {
                mncl_kv_foreach(top->value.object, free_resource_type, resclasses[i]);
            }
        }
        mncl_free_data(resmap);
    }
}

static void
delete_value (const char *key, void *val, void *user)
{
    (void)key;
    if (val && user) {
        MNCL_KV_DELETER deleter = (MNCL_KV_DELETER)user;
        deleter(val);
    }
}

void
mncl_unload_all_resources(void)
{
    int i;
    for (i = 0; resclasses[i]; ++i) {
        mncl_kv_foreach(&resclasses[i]->values, delete_value, resclasses[i]->values.deleter);
        tree_postorder(&(resclasses[i]->values.tree), (TREE_VISITOR)free);
        resclasses[i]->values.tree.root = NULL;
    }
    mncl_uninit_traits();
}

/* Locators */

MNCL_RAW *
mncl_raw_resource(const char *resource)
{
    return (MNCL_RAW *)mncl_kv_find(&raw.values, resource);
}

MNCL_SPRITESHEET *
mncl_spritesheet_resource(const char *resource)
{
    return (MNCL_SPRITESHEET *)mncl_kv_find(&spritesheet.values, resource);
}

MNCL_SPRITE *
mncl_sprite_resource(const char *resource)
{
    return (MNCL_SPRITE *)mncl_kv_find(&sprite.values, resource);
}

MNCL_FONT *
mncl_font_resource(const char *resource)
{
    return (MNCL_FONT *)mncl_kv_find(&font.values, resource);
}

MNCL_SFX *
mncl_sfx_resource(const char *resource)
{
    return (MNCL_SFX *)mncl_kv_find(&sfx.values, resource);
}

MNCL_DATA *
mncl_data_resource(const char *resource)
{
    return (MNCL_DATA *)mncl_kv_find(&data.values, resource);
}

MNCL_KIND *
mncl_kind_resource(const char *resource)
{
    return (MNCL_KIND *)mncl_kv_find(&kind.values, resource);
}

void
mncl_play_music_resource(const char *resource, int fade_in_ms)
{
    char *musicval = (char *)mncl_kv_find(&music.values, resource);
    
    if (musicval) {
        mncl_play_music_file(musicval, fade_in_ms);
    }
}

/* One could imagine reworking this into a more generalized
 * resource visitor, but this is the only case that the engine
 * really has to do this behind the user's back */

static void
normalize_visitor (const char *key, void *spritesheet, void *ignored)
{
    (void)key;
    (void)ignored;
    mncl_normalize_spritesheet((MNCL_SPRITESHEET *)spritesheet);
}

void
mncl_renormalize_all_spritesheets(void)
{
    mncl_kv_foreach(&spritesheet.values, normalize_visitor, NULL);
}
