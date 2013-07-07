#ifndef TREE_H_
#define TREE_H_

/**********************************************************************
 * tree.h - binary search trees
 *
 * This header file provides balanced and unbalanced binary search
 * trees. Balanced trees are provided via red-black trees via
 * red-black trees, an algorithm that is O(lg n) on all operations and
 * guarantees that no node will have one subtree more than twice as
 * deep as the other.
 *
 **********************************************************************/

/**********************************************************************
 * Tree node structure. Structures that you want to keep in a tree
 * should be prefaced with these, or have a TREE_NODE as the first
 * field.
 **********************************************************************/
typedef struct tree_node {
    struct tree_node *parent, *left, *right;
    char color;
} TREE_NODE;

/**********************************************************************
 * A generic tree pointer. This actually doesn't care what kind of
 * tree it is.
 **********************************************************************/
typedef struct tree_core {
    struct tree_node *root;
} TREE;

/**********************************************************************
 * Insert or find elements in the tree. Insert does not require unique
 * keys, but will maintain "stability" - that is, items that are
 * inserted later but are equal to elements currently in the tree will
 * show up later in an inorder traversal.  Find will return the first
 * element in the tree that matches the element you pass in, or NULL
 * if no such element exists.
 *
 * These functions take a comparator argument. This is a function f(a,
 * b) that with strcmp-like semantics: it returns a number less than 0
 * if a < b, equal to zero if a == b, and greater than zero if a > b.
 *
 * IMPORTANT! All calls within a tree to insert and find should use
 * the same comparator.
 *
 * Thanks to the way binary trees work, insert and find are the *only*
 * functions that actually require comparators. Everyone else just
 * relies on the binary tree properties.
 **********************************************************************/
typedef int (*TREE_CMP)(TREE_NODE *, TREE_NODE *);
TREE_NODE *tree_insert(TREE *t, TREE_NODE *i, TREE_CMP cmp);
TREE_NODE *tree_find(TREE *t, TREE_NODE *i, TREE_CMP cmp);

/**********************************************************************
 * Deletes the specified node n from the tree t. n must be a node that
 * is part of t; it's best to have it be a value returned by
 * tree_find().
 **********************************************************************/
void tree_delete(TREE *t, TREE_NODE *n);

/**********************************************************************
 * "Cursor" functions for inorder or reverse inorder
 * traversal. Starting at tree_minimum and repeatedly calling
 * tree_next gives you all the elements in order; starting at
 * tree_maximum and repeatedly calling tree_prev gives you them in
 * reverse order.
 *
 * If there is no such element, these functions return null.
 **********************************************************************/
TREE_NODE *tree_minimum(TREE *t);
TREE_NODE *tree_maximum(TREE *t);
TREE_NODE *tree_next(TREE_NODE *n);
TREE_NODE *tree_prev(TREE_NODE *n);

/**********************************************************************
 * Traversal functions. you hand them a tree and a visitor function,
 * that visitor function is called in preorder, inorder, or postorder,
 * as needed.
 *
 * Preorder and postorder traversals consume stack proportional to the
 * depth of the tree (that is, the log of the number of elements);
 * inorder traversals are purely iterative.
 *
 * If you've malloc()ed all of your node elements, a cheap and easy
 * way to clean up a TREE *t is
 *
 *    tree_postorder(t, (TREE_VISITOR)free);
 *    free(t);
 *
 **********************************************************************/
typedef void (*TREE_VISITOR)(TREE_NODE *);
void tree_preorder(TREE *t, TREE_VISITOR visitor);
void tree_inorder(TREE *t, TREE_VISITOR visitor);
void tree_postorder(TREE *t, TREE_VISITOR visitor);

/**********************************************************************
 * If for some reason you do not want to respect red-black status in
 * the tree, uses these functions instead of tree_insert and
 * tree_delete. All Hell is likely to break loose if you mix these
 * with the red-black versions, though.
 *
 * These are very likely only going to be useful when unit-testing the
 * "colorless" parts of the tree implementation.
 *
 * Note that every function in this library *except* for insert and
 * delete works identically on balanced or unbalanced trees.
 **********************************************************************/
TREE_NODE *tree_insert_unbalanced(TREE *t, TREE_NODE *i, TREE_CMP cmp);
void tree_delete_unbalanced(TREE *t, TREE_NODE *n);

#endif
