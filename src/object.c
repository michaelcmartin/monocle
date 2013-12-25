#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "monocle.h"
#include "monocle_internal.h"
#include "tree.h"

typedef struct struct_mncl_object_full {
    MNCL_OBJECT object;
    int depth;
    MNCL_KIND *kind;
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

/* Master set of objects. This tree is the one that owns its object
 * pointers; all others free their node structures without freeing the
 * object contained therein. */
static TREE master;

/* Objects that are pending creation or destruction. It is not safe to
 * create or destroy objects until the current set of iterations
 * completes, so we have to keep the sets of pending operations ready
 * as create/destroy methods come in. We must also ensure that no
 * event is ever sent out to the user regarding an object pending
 * destruction, for its userdata element may refer to invalid
 * material. */
static TREE pending_creation, pending_destruction;

/* Master map of traits. Traits are immortal. The first time we
 * try to create or look up a trait, if this doesn't exist it will
 * be populated with the default traits. */

/* These map the strings the user or config file give to integers. */
static MNCL_KV *traits = NULL;
static intptr_t num_traits = 0;

/* These map the integers stored above back to the names, and also
 * to the objects that have that trait for efficient iteration */
typedef struct mncl_subscriber_set {
    const char *name;
    TREE objs;
} MNCL_SUBSCRIBER_SET;
static int indexed_traits = 0, trait_capacity = 0;
static MNCL_SUBSCRIBER_SET *subscribers = NULL;

/* The current point in whatever iteration we're doing */
static TREE_NODE *current_iter = NULL;

static void
ensure_basic_traits(void)
{
    if (!traits) {
        traits = mncl_alloc_kv(NULL);
        num_traits = 0;
        mncl_kv_insert(traits, "invisible", (void *)++num_traits);
        mncl_kv_insert(traits, "pre-input", (void *)++num_traits);
        mncl_kv_insert(traits, "pre-physics", (void *)++num_traits);
        mncl_kv_insert(traits, "pre-render", (void *)++num_traits);
        mncl_kv_insert(traits, "render", (void *)++num_traits);
    }
}

void
initialize_object_trees(void)
{
    /* TODO: Confirm that we're starting from a position of no traits */
    master.root = NULL;
    pending_creation.root = NULL;
    pending_destruction.root = NULL;
    indexed_traits = 0;
    trait_capacity = 0;
    subscribers = NULL;
    ensure_basic_traits();
}

/* This is passed to mncl_kv_foreach below when we're setting up the
 * array that serves as the "traits" map's inverse */
static void
create_indexed_trait(const char *key, void *value, void *user)
{
    intptr_t trait_index = (intptr_t)value;
    if (trait_index < trait_capacity) {
        if (subscribers[trait_index].name == NULL) {
            subscribers[trait_index].name = key;
            subscribers[trait_index].objs.root = NULL;
        }
    } else {
        fprintf(stderr, "Tried to index an unallocated trait #%d for \"%s\"\n", (int)trait_index, key);
    }
}

/* This should really only be called after initialize_object_trees,
 * which should be automatic. */
/* Process pending creation and destruction events. Resize or create
 * the by-id trait array as needed. */
void
sync_object_trees(void)
{
    /* Do we have to change the amount of memory we've allocated? */
    if (num_traits > trait_capacity) {
        int i;
        if (trait_capacity == 0) {
            trait_capacity = 16;
        }
        while (trait_capacity < num_traits) {
            trait_capacity *= 2;
        }
        if (!subscribers) {
            subscribers = (MNCL_SUBSCRIBER_SET *)malloc(sizeof(MNCL_SUBSCRIBER_SET) * trait_capacity);
        } else {
            subscribers = (MNCL_SUBSCRIBER_SET *)realloc(subscribers, sizeof(MNCL_SUBSCRIBER_SET) * trait_capacity);
        }
        if (!subscribers) {
            fprintf(stderr, "Heap exhausted while organizing traits!\n");
            abort();
        }
        /* Now clear out the newly allocated entries */
        for (i = indexed_traits; i < trait_capacity; ++i) {
            subscribers[i].name = NULL;
            subscribers[i].objs.root = NULL;
        }
    }
    /* Do we need to create new trait categories? No already existing
     * objects can have a trait we haven't already indexed, but some
     * of the objects in pending_creation might. */
    if (num_traits > indexed_traits) {
        /* Sadly, we don't *have* a handy map from integers to numbers
         * yet. That's what we're building. So we'll have to iterate
         * through the traits indexed by name and look for stuff we've
         * previously not seen. */
        mncl_kv_foreach(traits, create_indexed_trait, NULL);
        indexed_traits = num_traits;
    }
    /* Process any newly created objects, filing them under the traits
     * listed in their kind. */
    if (pending_creation.root) {
        TREE_NODE *n = tree_minimum(&pending_creation);
        while (n) {
            MNCL_OBJECT_FULL *obj = ((MNCL_OBJECT_NODE *)n)->obj;
            unsigned int *kind_traits = obj->kind->traits;
            while (*kind_traits) {
                if (*kind_traits < indexed_traits) {
                    MNCL_OBJECT_NODE *new_node = (MNCL_OBJECT_NODE *)malloc(sizeof(MNCL_OBJECT_NODE));
                    if (!new_node) {
                        fprintf(stderr, "Heap exhaustion while organizing traits\n");
                        abort();
                    }
                    new_node->obj = obj;
                    tree_insert(&subscribers[*kind_traits].objs, (TREE_NODE *)new_node, objcmp);
                } else {
                    fprintf (stderr, "Target trait %d >= indexed trait count %d\n", *kind_traits, indexed_traits);
                }
                ++kind_traits;
            }
            n = tree_next(n);
        }
        /* Now clear out the space we'd been using to set these
         * up. The objects themselves live in "master" and so we don't
         * have to do anything to the contents.*/
        tree_postorder(&pending_creation, (TREE_VISITOR)free);
        pending_creation.root = NULL;
    }
    /* Process any newly destroyed objects, removing them from the
     * lists and cleaning up their memory. */
    if (pending_destruction.root) {
        TREE_NODE *n = tree_minimum(&pending_destruction);
        while (n) {
            MNCL_OBJECT_NODE search_node;
            TREE_NODE *found_node;
            MNCL_OBJECT_FULL *obj = ((MNCL_OBJECT_NODE *)n)->obj;
            unsigned int *kind_traits = obj->kind->traits;
            search_node.obj = obj;
            /* Unsubscribe from all the traits */
            while (*kind_traits) {
                if (*kind_traits < indexed_traits) {
                    found_node = tree_find(&subscribers[*kind_traits].objs, (TREE_NODE *)&search_node, objcmp);
                    if (found_node) {
                        tree_delete(&subscribers[*kind_traits].objs, found_node);
                        free(found_node);
                    }
                }
                ++kind_traits;
            }
            /* TODO: Other global sets, like the display list, once we get one */
            /* Now actually destroy the object proper, which is in the master tree */
            found_node = tree_find(&master, (TREE_NODE *)&search_node, objcmp);
            if (found_node) {
                MNCL_OBJECT_NODE *obj_node = (MNCL_OBJECT_NODE *)found_node;
                tree_delete(&master, found_node);
                free(obj_node->obj);
                free(found_node);
            }
            n = tree_next(n);
        }
        /* Now clear out the space we'd been using to set these
         * up. The objects themselves live in "master" and so we don't
         * have to do anything to the contents.*/
        tree_postorder(&pending_destruction, (TREE_VISITOR)free);
        pending_destruction.root = NULL;
    }
}

unsigned int
mncl_get_trait(const char *trait)
{
    intptr_t result;
    ensure_basic_traits();
    result = (intptr_t)mncl_kv_find(traits, trait);
    if (result) {
        return (unsigned int)result;
    }
    mncl_kv_insert(traits, trait, (void *)++num_traits);
    return num_traits;
}

void
mncl_uninit_traits(void)
{
    mncl_free_kv(traits);
    traits = NULL;
    num_traits = 0;
}

MNCL_OBJECT *
mncl_create_object(float x, float y, const char *kind)
{
    MNCL_OBJECT_FULL *obj = malloc(sizeof(MNCL_OBJECT_FULL));
    MNCL_OBJECT_NODE *node = malloc(sizeof(MNCL_OBJECT_NODE));
    MNCL_OBJECT_NODE *node2 = malloc(sizeof(MNCL_OBJECT_NODE));
    MNCL_KIND *k = mncl_kind_resource(kind);
    if (obj && node && node2 && k) {
        node->obj = obj;
        node2->obj = obj;
        tree_insert(&master, (TREE_NODE *)node, objcmp);
        tree_insert(&pending_creation, (TREE_NODE *)node2, objcmp);
        obj->object.x = x;
        obj->object.y = y;
        obj->object.f = 0;
        obj->object.dx = k->dx;
        obj->object.dx = k->dy;
        obj->object.df = k->df;
        obj->object.sprite = k->sprite;
        obj->depth = k->depth;
        obj->kind = k;
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
        if (!k) {
            fprintf(stderr, "Unknown kind \"%s\"\n", kind);
        }
        return NULL;
    }
    return &(obj->object);
}

MNCL_OBJECT *
object_begin(MNCL_EVENT_TYPE which)
{
    sync_object_trees();
    /* TODO: Cache these results */
    switch(which) {
    case MNCL_EVENT_PREINPUT:
        current_iter = tree_minimum(&subscribers[mncl_get_trait("pre-input")].objs);
        break;
    case MNCL_EVENT_PREPHYSICS:
        current_iter = tree_minimum(&subscribers[mncl_get_trait("pre-physics")].objs);
        break;
    case MNCL_EVENT_PRERENDER:
        current_iter = tree_minimum(&subscribers[mncl_get_trait("pre-render")].objs);
        break;
    default:
        /* TODO: MNCL_EVENT_RENDER ought to do display-list iteration */
        current_iter = NULL;
        break;
    }
    if (current_iter) {
        return &((MNCL_OBJECT_NODE *)current_iter)->obj->object;
    }
    return NULL;
}

MNCL_OBJECT *
object_next(void)
{
    while (current_iter) {
        current_iter = tree_next(current_iter);
        if (!tree_find(&pending_destruction, current_iter, objcmp)) {
            break;
        }
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
