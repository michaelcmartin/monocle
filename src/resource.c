#include <stdio.h>
#include <string.h>
#include "monocle.h"
#include "tree.h"
#include "json.h"

typedef void *(*ALLOC_FN)(JSON_VALUE *);

typedef struct res_class {
    MNCL_KV values;
    const char *type;
    ALLOC_FN alloc_fn;
} RES_CLASS;

static void *
raw_alloc(JSON_VALUE *arg) 
{
    if (!arg || arg->tag != JSON_STRING) {
        return NULL;
    }
    return mncl_acquire_raw(arg->value.string);
}

static void *
spritesheet_alloc(JSON_VALUE *arg)
{
    if (arg && arg->tag == JSON_STRING) {
        return mncl_alloc_spritesheet(arg->value.string);
    }
    return NULL;
}

static void *
sprite_alloc(JSON_VALUE *arg)
{
    if (arg && arg->tag == JSON_OBJECT) {
        JSON_VALUE *json_w = json_lookup(arg, "width");
        JSON_VALUE *json_h = json_lookup(arg, "height");
        JSON_VALUE *json_fr = json_lookup(arg, "frames");
        JSON_VALUE *hot_x = json_lookup(arg, "hotspot-x");
        JSON_VALUE *hot_y = json_lookup(arg, "hotspot-y");
        JSON_VALUE *hit_x = json_lookup(arg, "hitbox-x");
        JSON_VALUE *hit_y = json_lookup(arg, "hitbox-y");
        JSON_VALUE *hit_w = json_lookup(arg, "hitbox-width");
        JSON_VALUE *hit_h = json_lookup(arg, "hitbox-height");
                
        MNCL_SPRITE *result = NULL;
        int i;
        if (!(json_w && json_h && json_fr)) {
            return NULL;
        }
        if (json_w->tag != JSON_NUMBER || json_h->tag != JSON_NUMBER) {
            return NULL;
        }
        if (json_fr->tag != JSON_ARRAY) {
            return NULL;
        }
        result = mncl_alloc_sprite(json_fr->value.array.size);
        if (!result) {
            return NULL;
        }
        result->w = (int)json_w->value.number;
        result->h = (int)json_h->value.number;
        /* Some sensible defaults for optional values */
        result->hot_x = result->hot_y = 0;
        result->hit_x = result->hit_y = 0;
        result->hit_w = result->w;
        result->hit_h = result->h;
        /* Then override those if they were in the object */
        if (hot_x && hot_x->tag == JSON_NUMBER) {
            result->hot_x = (int)hot_x->value.number;
        }
        if (hot_y && hot_y->tag == JSON_NUMBER) {
            result->hot_y = (int)hot_y->value.number;
        }
        if (hit_x && hit_x->tag == JSON_NUMBER) {
            result->hit_x = (int)hit_x->value.number;
        }
        if (hit_y && hit_y->tag == JSON_NUMBER) {
            result->hit_y = (int)hit_y->value.number;
        }
        if (hit_w && hit_w->tag == JSON_NUMBER) {
            result->hit_w = (int)hit_w->value.number;
        }
        if (hit_h && hit_h->tag == JSON_NUMBER) {
            result->hit_h = (int)hit_h->value.number;
        }
        for (i = 0; i < result->nframes; ++i) {
            JSON_VALUE *json_f = json_fr->value.array.data[i];
            if (json_f && json_f->tag == JSON_OBJECT) {
                JSON_VALUE *x = json_lookup(json_f, "x");
                JSON_VALUE *y = json_lookup(json_f, "y");
                JSON_VALUE *ss = json_lookup(json_f, "spritesheet");
                if (x && y && ss && x->tag == JSON_NUMBER && y->tag == JSON_NUMBER && ss->tag == JSON_STRING) {
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
sfx_alloc(JSON_VALUE *arg)
{
    if (arg && arg->tag == JSON_STRING) {
        return mncl_alloc_sfx(arg->value.string);
    }
    return NULL;
}

static void *
music_alloc(JSON_VALUE *arg) 
{
    if (arg && arg->tag == JSON_STRING) {
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
    void *val = rc->alloc_fn((JSON_VALUE *)value);
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
    JSON_VALUE *resmap = NULL;
    MNCL_RAW *resmap_file = mncl_acquire_raw(path);
    if (!resmap_file) {
        return;
    }
    resmap = json_parse((char *)resmap_file->data, resmap_file->size);
    mncl_release_raw(resmap_file);
    if (resmap) {
        int i;
        for (i = 0; resclasses[i]; ++i) {
            JSON_VALUE *top = json_lookup(resmap, resclasses[i]->type);
            if (top && top->tag == JSON_OBJECT) {
                mncl_kv_foreach(top->value.object, alloc_resource_type, resclasses[i]);
            }
        }
    }
}

/* Unloaders */

void
mncl_unload_resmap(const char *path)
{
    JSON_VALUE *resmap = NULL;
    MNCL_RAW *resmap_file = mncl_acquire_raw(path);
    if (!resmap_file) {
        return;
    }
    resmap = json_parse((char *)resmap_file->data, resmap_file->size);
    mncl_release_raw(resmap_file);
    if (resmap) {
        int i;
        for (i = 0; resclasses[i]; ++i) {
            JSON_VALUE *top = json_lookup(resmap, resclasses[i]->type);
            if (top && top->tag == JSON_OBJECT) {
                mncl_kv_foreach(top->value.object, free_resource_type, resclasses[i]);
            }
        }
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
