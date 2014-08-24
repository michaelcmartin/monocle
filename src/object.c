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

/* This is like objcmp, but it's for the rendering tree. Sort first by
 * the depth of the object, and only for objects of identical depth
 * should we compare pointers. */
static int
scenecmp(TREE_NODE *a, TREE_NODE *b)
{
    MNCL_OBJECT_FULL *a_obj = (MNCL_OBJECT_FULL *)((MNCL_OBJECT_NODE *)a)->obj;
    MNCL_OBJECT_FULL *b_obj = (MNCL_OBJECT_FULL *)((MNCL_OBJECT_NODE *)b)->obj;
    int depth_diff = a_obj->depth - b_obj->depth;
    if (!depth_diff) {
        intptr_t a_ = (intptr_t)(&(a_obj->object));
        intptr_t b_ = (intptr_t)(&(b_obj->object));
        /* As above, mess around with ternary here because we don't
         * know if pointer differences at these ranges will fit in an
         * int. */
        return (a_ < b_) ? -1 : ((a_ > b_) ? 1 : 0);
    }
    return depth_diff;
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

/* Objects that get drawn. It is not safe to alter the "depth" field
 * of any of these objects except via set_object_depth, which properly
 * keeps this structure sorted. It is not safe to alter the depth of
 * objects during the rendering phase. */
static TREE renderable;

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

/* Collision iteration also requires us to track where we are in the
 * trait array and where we are in the iterated trait */
static TREE_NODE *collision_other_iter = NULL;
static unsigned int *collision_trait_iter = NULL;

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
        mncl_kv_insert(traits, "collision", (void *)++num_traits);
    }
}

void
initialize_object_trees(void)
{
    /* TODO: Confirm that we're starting from a position of no traits */
    master.root = NULL;
    pending_creation.root = NULL;
    pending_destruction.root = NULL;
    renderable.root = NULL;
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
        indexed_traits = num_traits + 1;
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
            /* Register for collisions if neccessary */
            if (obj->kind->collisions && *obj->kind->collisions) {
                MNCL_OBJECT_NODE *new_node = (MNCL_OBJECT_NODE *)malloc(sizeof(MNCL_OBJECT_NODE));
                if (!new_node) {
                    fprintf(stderr, "Heap exhaustion while organizing traits\n");
                    abort();
                }
                new_node->obj = obj;
                tree_insert(&subscribers[mncl_get_trait("collision")].objs, (TREE_NODE *)new_node, objcmp);
            }
            /* Register for rendering if necessary */
            if (obj->kind->visible) {
                MNCL_OBJECT_NODE *new_node = malloc(sizeof(MNCL_OBJECT_NODE));
                if (new_node) {
                    new_node->obj = obj;
                    tree_insert(&renderable, (TREE_NODE *)new_node, scenecmp);
                }
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
            /* Remove from the display list */
            found_node = tree_find(&renderable, (TREE_NODE *)&search_node, scenecmp);
            if (found_node) {
                tree_delete(&renderable, found_node);
                free(found_node);
            }

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

void
mncl_destroy_object(MNCL_OBJECT *obj)
{
    if (obj) {
        MNCL_OBJECT_NODE *node = malloc(sizeof(MNCL_OBJECT_NODE));
        if (node) {
            node->obj = (MNCL_OBJECT_FULL *)obj;
            tree_insert(&pending_destruction, (TREE_NODE *)node, objcmp);
        }
    }
}

MNCL_OBJECT *
object_begin(MNCL_EVENT_TYPE which)
{
    /* sync_object_trees() will clear out pending_destruction so we
     * don't have to check it here like we do in object_next() */
    sync_object_trees();
    /* TODO: Cache these results, maybe, for speed? */
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
        if (current_iter && !tree_find(&pending_destruction, current_iter, objcmp)) {
            return &((MNCL_OBJECT_NODE *)current_iter)->obj->object;
        }
    }
    return NULL;
}

void
collision_begin(MNCL_COLLISION *collision) {
    sync_object_trees();
    current_iter = tree_minimum(&subscribers[mncl_get_trait("collision")].objs);
    if (current_iter) {
        collision_trait_iter = ((MNCL_OBJECT_NODE *)current_iter)->obj->kind->collisions;
        if (*collision_trait_iter) {
            collision_other_iter = tree_minimum(&subscribers[*collision_trait_iter].objs);
        }
        collision_next(collision);
    } else {
        collision->self = collision->other = NULL;
    }
}

static void
collision_advance_iter(void)
{
    /* First, try to advance the inner iterator. */
    if (collision_other_iter) {
        collision_other_iter = tree_next(collision_other_iter);
        if (collision_other_iter) {
            /* We're pointing at a testable pair of objects */
            return;
        }
    }

    /* We're out of objects in this trait.  Advance the trait iterator
     * until we we find one with entries, or until we run out of
     * traits. */
    while (*collision_trait_iter) {
        ++collision_trait_iter;
        if (*collision_trait_iter) {
            collision_other_iter = tree_minimum(&subscribers[*collision_trait_iter].objs);
            if (collision_other_iter) {
                return;
            }
        }
    }

    /* We're out of traits, and thus we're out of things to test
     * current_iter against. On to the next object with things to
     * collide with. */
    while (current_iter) {
        current_iter = tree_next(current_iter);
        if (current_iter) {
            collision_trait_iter = ((MNCL_OBJECT_NODE *)current_iter)->obj->kind->collisions;
            while (*collision_trait_iter) {
                collision_other_iter = tree_minimum(&subscribers[*collision_trait_iter].objs);
                if (collision_other_iter) {
                    return;
                }
                ++collision_trait_iter;
            }
        }
    }
    
    /* We're out of objects. The iteration is over. */
}

void
collision_next(MNCL_COLLISION *collision)
{
    while (current_iter) {
        /* First, check to ensure that we can test these objects; one
         * or both might have been destroyed by processing the
         * previous event */
        if (collision_other_iter &&
            !tree_find(&pending_destruction, current_iter, objcmp) &&
            !tree_find(&pending_destruction, collision_other_iter, objcmp)) {
            MNCL_OBJECT *self = &((MNCL_OBJECT_NODE *)current_iter)->obj->object;
            MNCL_OBJECT *other = &((MNCL_OBJECT_NODE *)collision_other_iter)->obj->object;
            if (self != other) {
                /* For now we've only got box-style collisions */
                float s_left = self->x + self->sprite->hit_x;
                float s_right = self->x + self->sprite->hit_x + self->sprite->hit_w;
                float s_top = self->y + self->sprite->hit_y;
                float s_bottom = self->y + self->sprite->hit_y + self->sprite->hit_h;
                float o_left = other->x + other->sprite->hit_x;
                float o_right = other->x + other->sprite->hit_x + other->sprite->hit_w;
                float o_top = other->y + other->sprite->hit_y;
                float o_bottom = other->y + other->sprite->hit_y + other->sprite->hit_h;

                if ((s_right > o_left) && (s_left < o_right) &&
                    (s_bottom > o_top) && (s_top < o_bottom)) {
                    collision->self = self;
                    collision->other = other;
                    collision->trait_id = *collision_trait_iter;
                    collision->trait = subscribers[*collision_trait_iter].name;
                    collision_advance_iter();
                    return;
                }              
            }  
        }
        collision_advance_iter();
    }
    /* If we got here, we fell off the end of the iteration. */
    collision->self = collision->other = NULL;
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

void
mncl_object_set_depth(MNCL_OBJECT *o, int new_depth)
{
    MNCL_OBJECT_FULL *o_full = (MNCL_OBJECT_FULL *)o;
    MNCL_OBJECT_NODE search;
    TREE_NODE *render_node;
    search.obj = o_full;
    render_node = tree_find(&renderable, (TREE_NODE *)&search, scenecmp);
    if (render_node) {
        tree_delete(&renderable, render_node);
        o_full->depth = new_depth;
        tree_insert(&renderable, render_node, scenecmp);
    }
}

static MNCL_OBJECT *
render_process(void)
{
    while (current_iter) {
        MNCL_OBJECT_FULL *o_full = ((MNCL_OBJECT_NODE *)current_iter)->obj;
        MNCL_OBJECT *o = &(o_full->object);
        if (o_full->kind->visible && o->sprite && o->sprite->nframes &&
                !tree_find(&pending_destruction, current_iter, objcmp)) {
            if (o_full->kind->customrender) {
                return o;
            }
            mncl_draw_sprite(o->sprite, (int)o->x, (int)o->y, (int)o->f);
        }
        current_iter = tree_prev(current_iter);
    }
    return NULL;
}

MNCL_OBJECT *
render_begin(void)
{
    current_iter = tree_maximum(&renderable);
    return render_process();
}

MNCL_OBJECT *
render_next(void)
{
    current_iter = tree_prev(current_iter);
    return render_process();
}
