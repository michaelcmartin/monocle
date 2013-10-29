#include <stdio.h>
#include <string.h>
#include "monocle.h"
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
        result->hit_x = result->hit_y = 0;
        result->hit_w = result->w;
        result->hit_h = result->h;
        /* Then override those if they were in the object */
        if (hot_x && hot_x->tag == MNCL_DATA_NUMBER) {
            result->hot_x = (int)hot_x->value.number;
        }
        if (hot_y && hot_y->tag == MNCL_DATA_NUMBER) {
            result->hot_y = (int)hot_y->value.number;
        }
        if (hit_x && hit_x->tag == MNCL_DATA_NUMBER) {
            result->hit_x = (int)hit_x->value.number;
        }
        if (hit_y && hit_y->tag == MNCL_DATA_NUMBER) {
            result->hit_y = (int)hit_y->value.number;
        }
        if (hit_w && hit_w->tag == MNCL_DATA_NUMBER) {
            result->hit_w = (int)hit_w->value.number;
        }
        if (hit_h && hit_h->tag == MNCL_DATA_NUMBER) {
            result->hit_h = (int)hit_h->value.number;
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

static RES_CLASS raw = { { { NULL }, (MNCL_KV_DELETER)mncl_release_raw }, "raw", raw_alloc };
static RES_CLASS spritesheet = { { { NULL }, (MNCL_KV_DELETER)mncl_free_spritesheet }, "spritesheet", spritesheet_alloc };
static RES_CLASS sprite = { { { NULL }, (MNCL_KV_DELETER)mncl_free_sprite },  "sprite", sprite_alloc };
static RES_CLASS sfx = { { { NULL }, (MNCL_KV_DELETER)mncl_free_sfx }, "sfx", sfx_alloc };
static RES_CLASS music = { { { NULL }, free }, "music", music_alloc };

static RES_CLASS *resclasses[] = { &raw, &spritesheet, &sprite, &sfx, &music, NULL };

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

MNCL_SFX *
mncl_sfx_resource(const char *resource)
{
    return (MNCL_SFX *)mncl_kv_find(&sfx.values, resource);
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
