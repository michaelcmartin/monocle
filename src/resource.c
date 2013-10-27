#include <stdio.h>
#include <string.h>
#include "monocle.h"
#include "tree.h"
#include "json.h"

/* Possible TODO here: refactor all this to use MNCL_KV instead of
 * TREE directly */

typedef void *(*ALLOC_FN)(JSON_VALUE *);
typedef void (*FREE_FN)(void *);

typedef struct res_class {
    TREE values;
    const char *type;
    ALLOC_FN alloc_fn;
    FREE_FN free_fn;
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

static RES_CLASS raw = { { NULL }, "raw", raw_alloc, (FREE_FN)mncl_release_raw };
static RES_CLASS spritesheet={ { NULL }, "spritesheet", spritesheet_alloc, (FREE_FN)mncl_free_spritesheet };
static RES_CLASS sprite = { { NULL }, "sprite", sprite_alloc, (FREE_FN)mncl_free_sprite };
static RES_CLASS sfx = { { NULL }, "sfx", sfx_alloc, (FREE_FN)mncl_free_sfx };
static RES_CLASS music = { { NULL }, "music", music_alloc, free };

static RES_CLASS *resclasses[] = { &raw, &spritesheet, &sprite, &sfx, &music, NULL };

static void
process_resource_type(JSON_VALUE *resmap, RES_CLASS *rc, int allocating)
{
    JSON_VALUE *top = json_lookup(resmap, rc->type);
    if (top && top->tag == JSON_OBJECT) {
        KEY_VALUE_NODE *i = (KEY_VALUE_NODE *)tree_minimum(&top->value.object);
        while (i) {
            void *val = allocating ? rc->alloc_fn((JSON_VALUE *)i->value) : NULL;
            if (val || !allocating) {
                KEY_SEARCH_NODE seek;
                KEY_VALUE_NODE *result;
                seek.key=i->key;
                result = (KEY_VALUE_NODE *)tree_find(&rc->values, (TREE_NODE *)&seek, key_value_node_cmp);
                if (result) {
                    rc->free_fn(result->value);
                    if (allocating) {
                        printf("WARNING: overwriting %s resource %s\n", rc->type, i->key);
                        result->value = val;
                    }
                } else {
                    if (allocating) {
                        result = key_value_node_alloc(i->key, val);
                        if (!result) {
                            printf("WARNING: Could not store %s resource %s\n", rc->type, i->key);
                            rc->free_fn(val);
                        } else {
                            printf("Loading %s resource %s\n", rc->type, result->key);
                            tree_insert(&rc->values, (TREE_NODE *)result, key_value_node_cmp);
                        }
                    } else {
                        printf("WARNING: %s resource %s has already been deleted\n", rc->type, i->key);
                    }
                }
            } else {
                printf("WARNING: Could not load %s resource %s\n", rc->type, i->key);
            }
            i = (KEY_VALUE_NODE *)tree_next((TREE_NODE *)i);
        }
    } else {
        printf("WARNING: No %s resources\n", rc->type);
    }
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
            process_resource_type(resmap, resclasses[i], 1);
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
            process_resource_type(resmap, resclasses[i], 0);
        }
    }
}

void
mncl_unload_all_resources(void)
{
    int i;
    for (i = 0; resclasses[i]; ++i) {
        TREE_NODE *node = tree_minimum(&(resclasses[i]->values));
        while (node) {
            resclasses[i]->free_fn(((KEY_VALUE_NODE *)node)->value);
            node = tree_next(node);
        }
        tree_postorder(&(resclasses[i]->values), (TREE_VISITOR)free);
        resclasses[i]->values.root = NULL;
    }
}

/* Locators */

static void *
find_res(TREE *map, const char *key)
{
    KEY_SEARCH_NODE seek;
    KEY_VALUE_NODE *result;
    seek.key = key;
    result = (KEY_VALUE_NODE *)tree_find(map, (TREE_NODE *)&seek, key_value_node_cmp);
    if (result) {
        return result->value;
    }
    printf("Not found\n");
    return NULL;
}

MNCL_RAW *
mncl_raw_resource(const char *resource)
{
    return (MNCL_RAW *)find_res(&raw.values, resource);
}

MNCL_SPRITESHEET *
mncl_spritesheet_resource(const char *resource)
{
    return (MNCL_SPRITESHEET *)find_res(&spritesheet.values, resource);
}

MNCL_SPRITE *
mncl_sprite_resource(const char *resource)
{
    return (MNCL_SPRITE *)find_res(&sprite.values, resource);
}

MNCL_SFX *
mncl_sfx_resource(const char *resource)
{
    return (MNCL_SFX *)find_res(&sfx.values, resource);
}

void
mncl_play_music_resource(const char *resource, int fade_in_ms)
{
    char *musicval = (char *)find_res(&music.values, resource);
    
    if (musicval) {
        mncl_play_music_file(musicval, fade_in_ms);
    }
}

/* One could imagine reworking this into a more generalized
 * resource visitor, but this is the only case that the engine
 * really has to do this behind the user's back */

void
mncl_renormalize_all_spritesheets(void)
{
    TREE_NODE *node = tree_minimum(&spritesheet.values);
    while (node) {
        mncl_normalize_spritesheet((MNCL_SPRITESHEET *)(((KEY_VALUE_NODE *)node)->value));
        node = tree_next(node);
    }
}
