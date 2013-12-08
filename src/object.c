#include <math.h>
#include "monocle.h"
#include "monocle_internal.h"
#include "tree.h"

typedef struct struct_mncl_object_full {
    MNCL_OBJECT object;
    int depth;
    int visible;
} MNCL_OBJECT_FULL;

typedef struct struct_mncl_object_node {
    TREE_NODE header;
    MNCL_OBJECT_FULL *obj;
} MNCL_OBJECT_NODE;

static int
objcmp(TREE_NODE *a, TREE_NODE *b)
{
    intptr_t a_ = (intptr_t)(&((MNCL_OBJECT_NODE *)a)->obj->object);
    intptr_t b_ = (intptr_t)(&((MNCL_OBJECT_NODE *)b)->obj->object);
    /* Mess around with ternary here because we don't know if pointer
     * differences at these ranges will fit in an int. */
    return (a_ < b_) ? -1 : ((a_ > b_) ? 1 : 0);
}

/* TODO: These will probably need some reworking once kinds become a
 * thing */

/* Master set of objects. This tree is the one that owns its object
 * pointers; all others free their node structures without freeing the
 * object contained therein. */
static TREE master;

/* Subscription trees for individual events. Not all of these events
 * are subscribable, but this makes the enumerated types work more
 * readily. */
static TREE subscribed[MNCL_NUM_EVENTS];

/* The current point in whatever iteration we're doing */
static TREE_NODE *current_iter = NULL;

void
initialize_object_trees(void)
{
    int i;
    master.root = NULL;
    for (i = 0; i < MNCL_NUM_EVENTS; ++i) {
        subscribed[i].root = NULL;
    }
}

MNCL_OBJECT *
mncl_create_object(void)
{
    MNCL_OBJECT_FULL *obj = malloc(sizeof(MNCL_OBJECT_FULL));
    MNCL_OBJECT_NODE *node = malloc(sizeof(MNCL_OBJECT_NODE));
    /* We actually need one of these per subscription, in the end. */
    MNCL_OBJECT_NODE *node2 = malloc(sizeof(MNCL_OBJECT_NODE));
    if (obj && node && node2) {
        node->obj = obj;
        node2->obj = obj;
        tree_insert(&master, (TREE_NODE *)node, objcmp);
        tree_insert(&subscribed[MNCL_EVENT_PRERENDER], (TREE_NODE *)node2, objcmp);
    } else {
        if (obj) {
            free(obj);
        }
        if (node) {
            free(node);
        }
        if (node2) {
            free(node2);
        }
        return NULL;
    }
    return &(obj->object);
}

MNCL_OBJECT *
object_begin(MNCL_EVENT_TYPE which)
{
    if (which < 0 || which >= MNCL_NUM_EVENTS) {
        current_iter = NULL;
    } else {
        current_iter = tree_minimum(&subscribed[which]);
    }
    if (current_iter) {
        return &((MNCL_OBJECT_NODE *)current_iter)->obj->object;
    }
    return NULL;
}

MNCL_OBJECT *
object_next(void)
{
    if (current_iter) {
        current_iter = tree_next(current_iter);
    }
    if (current_iter) {
        return &((MNCL_OBJECT_NODE *)current_iter)->obj->object;
    }
    return NULL;
}

void
default_update_all_objects(void)
{
    TREE_NODE *n = tree_minimum(&master);
    while (n) {
        MNCL_OBJECT *o = &(((MNCL_OBJECT_NODE *)n)->obj->object);
        o->x += o->dx;
        o->y += o->dy;
        o->f += o->df;
        if (o->sprite && o->sprite->nframes) {
            o->f = fmod(o->f, o->sprite->nframes);
            if (o->f < 0) {
                o->f = o->sprite->nframes - o->f;
            }
        }
        n = tree_next(n);
    }
}

/* THIS IS A TEMPORARY FUNCTION; IT WILL GO AWAY ONCE TRAITS ARE
 * IMPLEMENTED, AND IN PARTICULAR, THE INVISIBLE AND CUSTOMRENDER
 * TRAITS */
void
default_render_all_objects(void)
{
    TREE_NODE *n = tree_minimum(&master);
    while (n) {
        MNCL_OBJECT *o = &(((MNCL_OBJECT_NODE *)n)->obj->object);
        if (o->sprite && o->sprite->nframes) {
            mncl_draw_sprite(o->sprite, (int)o->x, (int)o->y, (int)o->f);
        }
        n = tree_next(n);
    }
}
