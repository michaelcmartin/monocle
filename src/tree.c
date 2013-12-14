#include <stdlib.h>
#include <string.h>
#include "tree.h"
/**********************************************************************
 * tree.c - binary search trees implementation
 *
 * This implementation is largely based on the pseudocode provided in
 * Chapters 13 and 14 of _Introduction_to_Algorithms by Cormen,
 * Lieserson, and Rivest. For further verification, I have run test
 * cases that handle all the algorithms' special cases against
 * known-good answers and against FreeBSD's implementation in
 * sys/tree.h.
 *
 * It is reasonable to ask why one would re-invent the wheel when a
 * ready-made solution is already to hand. This solution is somewhat
 * simpler and thus more compact than the FreeBSD version; this also
 * will hopefully make it easier to use. The FreeBSD implementation is
 * built largely out of macros and generates code roughly similar to
 * C++ template expansion. This implementation is simpler and built in
 * a form more analogous to single inheritance. As long as you aren't
 * trying to make a single chunk of data be part of two separate
 * intrusive-pointer-based data structures, this should meet your
 * needs.
 **********************************************************************/

int
key_value_node_cmp(TREE_NODE *a, TREE_NODE *b)
{
    return strcmp(((KEY_VALUE_NODE *)a)->key, ((KEY_VALUE_NODE *)b)->key);
}

KEY_VALUE_NODE *
key_value_node_alloc(const char *key, void *value)
{
    int size = strlen(key)+1;
    KEY_VALUE_NODE *result = (KEY_VALUE_NODE *)malloc(sizeof(KEY_VALUE_NODE) + size);
    if (!result) {
        return NULL;
    }
    result->value = value;
    strncpy(result->data, key, size);
    result->key = &(result->data[0]);
    return result;
}

MNCL_KV *
mncl_alloc_kv(MNCL_KV_DELETER deleter)
{
    MNCL_KV *result = malloc(sizeof(MNCL_KV));
    if (!result) {
        return NULL;
    }
    result->tree.root = NULL;
    result->deleter = deleter;
    return result;
}

void
mncl_free_kv(MNCL_KV *kv)
{
    if (!kv) {
        return;
    }
    if (kv->deleter) {
        TREE_NODE *node = tree_minimum(&kv->tree);
        while (node) {
            kv->deleter(((KEY_VALUE_NODE *)node)->value);
            node = tree_next(node);
        }
    }
    tree_postorder (&kv->tree, (TREE_VISITOR)free);
    free(kv);
}

int
mncl_kv_insert(MNCL_KV *kv, const char *key, void *value)
{
    KEY_SEARCH_NODE seek;
    KEY_VALUE_NODE *result;
    if (!kv) {
        return 0;
    }
    seek.key=key;
    result = (KEY_VALUE_NODE *)tree_find(&kv->tree, (TREE_NODE *)&seek, key_value_node_cmp);
    if (result) {
        if (kv->deleter) {
            kv->deleter(result->value);
        }
        result->value = value;
    } else {
        result = key_value_node_alloc(key, value);
        if (!result) {
            return 0;
        } else {
            tree_insert(&kv->tree, (TREE_NODE *)result, key_value_node_cmp);
        }
    }
    return 1;
}

void *
mncl_kv_find(MNCL_KV *kv, const char *key)
{
    KEY_SEARCH_NODE seek;
    KEY_VALUE_NODE *result;
    if (!kv) {
        return NULL;
    }
    seek.key = key;
    result = (KEY_VALUE_NODE *)tree_find(&kv->tree, (TREE_NODE *)&seek, key_value_node_cmp);
    if (result) {
        return result->value;
    }
    return NULL;
}

void
mncl_kv_delete(MNCL_KV *kv, const char *key)
{
    KEY_SEARCH_NODE seek;
    KEY_VALUE_NODE *result;
    if (!kv) {
        return;
    }
    seek.key = key;
    result = (KEY_VALUE_NODE *)tree_find(&kv->tree, (TREE_NODE *)&seek, key_value_node_cmp);
    if (result) {
        if (kv->deleter) {
            kv->deleter(result->value);
        }
        tree_delete(&kv->tree, (TREE_NODE *)result);
        free(result);
    }
}

void
mncl_kv_foreach(MNCL_KV *kv, MNCL_KV_VALUE_FN fn, void *user)
{
    TREE_NODE *node;
    if (!kv) {
        return;
    }
    node = tree_minimum(&kv->tree);
    while (node) {
        KEY_VALUE_NODE *kvn = (KEY_VALUE_NODE *)node;
        fn(kvn->key, kvn->value, user);
        node = tree_next(node);
    }
}

TREE_NODE *
tree_minimum(TREE *t)
{
    TREE_NODE *n = t->root;
    if (!n) {
        return NULL;
    }
    while (n->left) {
        n = n->left;
    }
    return n;
}

TREE_NODE *
tree_maximum(TREE *t)
{
    TREE_NODE *n = t->root;
    if (!n) {
        return NULL;
    }
    while (n->right) {
        n = n->right;
    }
    return n;
}

TREE_NODE *
tree_next(TREE_NODE *n)
{
    if (n->right) {
        n = n->right;
        while (n->left) {
            n = n->left;
        }
        return n;
    }
    while (n) {
        if (n->parent && n->parent->left == n) {
            return n->parent;
        } else {
            n = n->parent;
        }
    }
    return NULL;
}

TREE_NODE *
tree_prev(TREE_NODE *n)
{
    if (n->left) {
        n = n->left;
        while (n->right) {
            n = n->right;
        }
        return n;
    }
    while (n) {
        if (n->parent && n->parent->right == n) {
            return n->parent;
        } else {
            n = n->parent;
        }
    }
    return NULL;
}

TREE_NODE *
tree_insert_unbalanced(TREE *t, TREE_NODE *i, TREE_CMP cmp)
{
    TREE_NODE **parent_ptr = &t->root;
    TREE_NODE *n = t->root;
    TREE_NODE *p = NULL;
    while (n) {
        int c = cmp(i, n);
        p = n;
        if (c < 0) {
            parent_ptr = &n->left;
            n = n->left;
        } else {
            parent_ptr = &n->right;
            n = n->right;
        }
    }
    *parent_ptr = i;
    i->left = NULL;
    i->right = NULL;
    i->parent = p;
    return i;
}

TREE_NODE *
tree_find(TREE *t, TREE_NODE *i, TREE_CMP cmp)
{
    TREE_NODE *n = t->root;
    while (n) {
        int c = cmp(i, n);
        if (c < 0) {
            n = n->left;
        } else if (c > 0) {
            n = n->right;
        } else {
            /* We found it! */
            break;
        }
    }
    return n;
}

void
tree_delete_unbalanced(TREE *t, TREE_NODE *n)
{
    TREE_NODE *splice_out, *orphan;
    if (!t || !t->root || !n) {
        return;
    }
    splice_out = (n->left && n->right) ? tree_next(n) : n;
    orphan = splice_out->left ? splice_out->left : splice_out->right;
    if (orphan) {
        orphan->parent = splice_out->parent;
    }
    if (!splice_out->parent) {
        t->root = orphan;
    } else if (splice_out->parent->left == splice_out) {
        splice_out->parent->left = orphan;
    } else {
        splice_out->parent->right = orphan;
    }
    if (splice_out != n) {
        if (!n->parent) {
            t->root = splice_out;
        } else if (n->parent->left == n) {
            n->parent->left = splice_out;
        } else {
            n->parent->right = splice_out;
        }
        splice_out->parent = n->parent;
        splice_out->left = n->left;
        splice_out->right = n->right;
        if (n->left) {
            n->left->parent = splice_out;
        }
        if (n->right) {
            n->right->parent = splice_out;
        }
    }
}

static void
tree_preorder_aux(TREE_NODE *n, TREE_VISITOR visitor)
{
    if (!n) {
        return;
    }
    visitor(n);
    tree_preorder_aux(n->left, visitor);
    tree_preorder_aux(n->right, visitor);
}

static void
tree_postorder_aux(TREE_NODE *n, TREE_VISITOR visitor)
{
    if (!n) {
        return;
    }
    tree_postorder_aux(n->left, visitor);
    tree_postorder_aux(n->right, visitor);
    visitor(n);
}

void
tree_preorder(TREE *t, TREE_VISITOR visitor)
{
    if (t) {
        tree_preorder_aux(t->root, visitor);
    }
}

void
tree_inorder(TREE *t, TREE_VISITOR visitor)
{
    TREE_NODE *n;
    if (!t || !t->root) {
        return;
    }
    n = t->root;
    while (n->left) {
        n = n->left;
    }
    while (n) {
        visitor(n);
        n = tree_next(n);
    }
}

void
tree_postorder(TREE *t, TREE_VISITOR visitor)
{
    if (t) {
        tree_postorder_aux(t->root, visitor);
    }
}

/* Red-black tree routines */

#define RB_RED   0
#define RB_BLACK 1

static void
left_rotate(TREE *t, TREE_NODE *n)
{
    TREE_NODE *o = n->right;
    if (!o) {
        return;
    }
    n->right = o->left;
    if (o->left) {
        o->left->parent = n;
    }
    o->parent = n->parent;
    if (!n->parent) {
        t->root = o;
    } else {
        if (n == n->parent->left) {
            n->parent->left = o;
        } else {
            n->parent->right = o;
        }
    }
    o->left = n;
    n->parent = o;
}

static void
right_rotate(TREE *t, TREE_NODE *n)
{
    TREE_NODE *o = n->left;
    if (!o) {
        return;
    }
    n->left = o->right;
    if (o->right) {
        o->right->parent = n;
    }
    o->parent = n->parent;
    if (!n->parent) {
        t->root = o;
    } else {
        if (n == n->parent->left) {
            n->parent->left = o;
        } else {
            n->parent->right = o;
        }
    }
    o->right = n;
    n->parent = o;
}

TREE_NODE *
tree_insert(TREE *t, TREE_NODE *i, TREE_CMP cmp)
{
    TREE_NODE *result = i;
    i = tree_insert_unbalanced(t, i, cmp);
    i->color = RB_RED;
    while (i->parent && i->parent->color == RB_RED) {
        TREE_NODE *grandparent, *uncle;
        /* Since the parent was red, it's not the root, so grandparent must exist */
        grandparent = i->parent->parent;
        if (i->parent == grandparent->left) {
            uncle = grandparent->right;
            if (uncle && uncle->color == RB_RED) {
                i->parent->color = RB_BLACK;
                uncle->color = RB_BLACK;
                grandparent->color = RB_RED;
                i = grandparent;
            } else {
                if (i == i->parent->right) {
                    i = i->parent;
                    left_rotate(t, i);
                }
                i->parent->color = RB_BLACK;
                grandparent->color = RB_RED;
                right_rotate(t, grandparent);
            }
        } else {
            uncle = grandparent->left;
            if (uncle && uncle->color == RB_RED) {
                i->parent->color = RB_BLACK;
                uncle->color = RB_BLACK;
                grandparent->color = RB_RED;
                i = grandparent;
            } else {
                if (i == i->parent->left) {
                    i = i->parent;
                    right_rotate(t, i);
                }
                i->parent->color = RB_BLACK;
                grandparent->color = RB_RED;
                left_rotate(t, grandparent);
            }
        }
    }
    t->root->color = RB_BLACK;
    return result;
}

/* This could probably be combined some with tree_delete_unbalanced, but the need
 * to track orphan_parent here complicates that */
void
tree_delete(TREE *t, TREE_NODE *n)
{
    TREE_NODE *splice_out, *orphan, *orphan_parent;
    int need_fixup = 0;
    if (!t || !t->root || !n) {
        return;
    }
    splice_out = (n->left && n->right) ? tree_next(n) : n;
    orphan = splice_out->left ? splice_out->left : splice_out->right;
    orphan_parent = splice_out->parent;
    if (orphan) {
        orphan->parent = orphan_parent;
    }
    if (!splice_out->parent) {
        t->root = orphan;
    } else if (splice_out->parent->left == splice_out) {
        splice_out->parent->left = orphan;
    } else {
        splice_out->parent->right = orphan;
    }
    if (splice_out->color == RB_BLACK) {
        need_fixup = 1;
    }
    if (splice_out != n) {
        if (!n->parent) {
            t->root = splice_out;
        } else if (n->parent->left == n) {
            n->parent->left = splice_out;
        } else {
            n->parent->right = splice_out;
        }
        splice_out->parent = n->parent;
        splice_out->left = n->left;
        splice_out->right = n->right;
        splice_out->color = n->color;
        if (orphan_parent == n) {
            orphan_parent = splice_out;
        }
        if (n->left) {
            n->left->parent = splice_out;
        }
        if (n->right) {
            n->right->parent = splice_out;
        }
    }
    if (need_fixup) {
        TREE_NODE *x, *x_parent, *sibling;
        x = orphan;
        x_parent = orphan_parent;
        while (x_parent && (!x || x->color == RB_BLACK)) {
            if (x == x_parent->left) {
                sibling = x_parent->right;
                if (sibling && sibling->color == RB_RED) {
                    sibling->color = RB_BLACK;
                    x_parent->color = RB_RED;
                    left_rotate(t, x_parent);
                    sibling = x_parent->right;
                }
                if ((!sibling->left || sibling->left->color == RB_BLACK) &&
                    (!sibling->right || sibling->right->color == RB_BLACK)) {
                    sibling->color = RB_RED;
                    x = x_parent;
                    x_parent = x ? x->parent : NULL;
                } else {
                    if (!sibling->right || sibling->right->color == RB_BLACK) {
                        /* sibling->left must exist and be RB_RED */
                        sibling->left->color = RB_RED;
                        right_rotate(t, sibling);
                        sibling = x_parent->right;
                    }
                    if (sibling) {
                        sibling->color = x_parent ? x_parent->color : RB_BLACK;
                    }
                    if (x_parent) {
                        x_parent->color = RB_BLACK;
                    }
                    if (sibling->right) {
                        sibling->right->color = RB_BLACK;
                    }
                    left_rotate(t, x_parent);
                    x = t->root;
                    x_parent = NULL;
                }
            } else {
                /* as above, but mirrored */
                sibling = x_parent->left;
                if (sibling && sibling->color == RB_RED) {
                    sibling->color = RB_BLACK;
                    x_parent->color = RB_RED;
                    right_rotate(t, x_parent);
                    sibling = x_parent->left;
                }
                if ((!sibling->left || sibling->left->color == RB_BLACK) &&
                    (!sibling->right || sibling->right->color == RB_BLACK)) {
                    sibling->color = RB_RED;
                    x = x_parent;
                    x_parent = x ? x->parent : NULL;
                } else {
                    if (!sibling->left || sibling->left->color == RB_BLACK) {
                        /* sibling->right must exist and be RB_RED */
                        sibling->right->color = RB_RED;
                        left_rotate(t, sibling);
                        sibling = x_parent->left;
                    }
                    if (sibling) {
                        sibling->color = x_parent ? x_parent->color : RB_BLACK;
                    }
                    if (x_parent) {
                        x_parent->color = RB_BLACK;
                    }
                    if (sibling->left) {
                        sibling->left->color = RB_BLACK;
                    }
                    right_rotate(t, x_parent);
                    x = t->root;
                    x_parent = NULL;
                }
            }
        }
        if (x) {
            x->color = RB_BLACK;
        }
    }       
}
