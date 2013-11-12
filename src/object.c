#include <math.h>

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

/* TODO: These will need some reworking once kinds become a thing */

/* Master set of objects. This tree is the one that owns its object
 * pointers; all others free their node structures without freeing the
 * object contained therein. */
static TREE master = { NULL };

MNCL_OBJECT *
mncl_create_object(int kind, int depth, int visible)
{
    MNCL_OBJECT_FULL *obj = malloc(sizeof(MNCL_OBJECT_FULL));
    MNCL_OBJECT_NODE *node = malloc(sizeof(MNCL_OBJECT_NODE));
    if (obj && node) {
        node->obj = obj;
        tree_insert(&master, (TREE_NODE *)node, objcmp);
    } else {
        if (obj) {
            free(obj);
        }
        if (node) {
            free(node);
        }
        return NULL;
    }
    (void)kind;
    return &(obj->object);
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
