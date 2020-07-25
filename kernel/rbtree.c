/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel/rbtree.h>

rbtree_node_t* _rbtree_deepest(rbtree_node_t *node) {
    if (node == NULL) return NULL;

    // Find the deepest point starting with the left subtree
    rbtree_node_t *parent = NULL;

    while (true) {
        parent = node;
        node = rbtree_left(parent);

        if (node == NULL) {
            // Check the right subtree
            node = rbtree_right(parent);

            // No subtrees
            if (node == NULL) return parent;
        }
    }
}

rbtree_node_t* _rbtree_emancipate(rbtree_node_t *child) {
    // Cut the link between a parent and it's child, return the parent
    rbtree_node_t *parent = rbtree_parent(child);
    rbtree_set_parent(child, NULL);

    if (parent != NULL) {
        if (child == rbtree_left(parent)) {
            rbtree_left(parent) = NULL;
        } else {
            rbtree_right(parent) = NULL;
        }
    }

    return parent;
}

rbtree_node_t* _rbtree_find_root(rbtree_node_t *node) {
    rbtree_node_t *root = NULL;

    while (node != NULL) {
        root = node;
        node = rbtree_parent(node);
    }

    return root;
}

void _rbtree_rotate(rbtree_t *tree, rbtree_node_t *node, rbtree_dir_t dir) {
    // The child must not be null
    rbtree_dir_t opposite_dir = dir ^ 1;
    rbtree_node_t *child = rbtree_this_child(node, dir ^ 1), *child_subtree = rbtree_this_child(child, dir);
    rbtree_node_t *parent = rbtree_parent(node), *parent_left = (parent != NULL) ? rbtree_left(parent) : NULL;

    // Cut the links between this node and it's parent and this node and it's to-be-rotated child
    _rbtree_emancipate(child);
    _rbtree_emancipate(node);

    // Make the child of node the new child of parent
    if (parent != NULL) {
        rbtree_child_t which_child = (parent_left == node) ? RBTREE_CHILD_LEFT : RBTREE_CHILD_RIGHT;
        rbtree_this_child(parent, which_child) = child;
        rbtree_set_parent(child, parent);
    } else {
        // If node was root, make child the new root
        rbtree_root(tree) = child;
    }

    // Depending on the rotation direction, make the left subtree of the right child of node the new right
    // subtree of node OR make the right subtree of the left child of node the new left subtree of node
    if (child_subtree != NULL) {
        _rbtree_emancipate(child_subtree);
        rbtree_this_child(node, opposite_dir) = child_subtree;
        rbtree_set_parent(child_subtree, node);
    }

    // Finally make node the new child of it's former child
    rbtree_this_child(child, dir) = node;
    rbtree_set_parent(node, child);
}

rbtree_node_t* rbtree_grandparent(rbtree_node_t *node) {
    if (node == NULL) return NULL;

    rbtree_node_t *grandparent = NULL;
    rbtree_node_t *parent = rbtree_parent(node);

    if (parent != NULL) {
        grandparent = rbtree_parent(parent);
    }

    return grandparent;
}

rbtree_node_t* rbtree_sibling(rbtree_node_t *node) {
    if (node == NULL) return NULL;

    rbtree_node_t *sibling = NULL;
    rbtree_node_t *parent = rbtree_parent(node);

    if (parent != NULL) {
        sibling = (node == rbtree_left(parent)) ? rbtree_right(parent) : rbtree_left(parent);
    }

    return sibling;
}

rbtree_node_t* rbtree_uncle(rbtree_node_t *node) {
    if (node == NULL) return NULL;

    rbtree_node_t *uncle = NULL;
    rbtree_node_t *parent = rbtree_parent(node);

    if (parent != NULL) {
        uncle = rbtree_sibling(parent);
    }

    return uncle;
}

rbtree_node_t* rbtree_node_max(rbtree_node_t *node) {
    // The max can be found by continuously traversing through the right child
    rbtree_node_t *max = NULL;

    for (rbtree_node_t *here = node; here != NULL; here = rbtree_right(here)) {
        max = here;
    }

    return max;
}

rbtree_node_t* rbtree_node_min(rbtree_node_t *node) {
    // The min can be found by continuously traversing through the left child
    rbtree_node_t *min = NULL;

    for (rbtree_node_t *here = node; here != NULL; here = rbtree_left(here)) {
        min = here;
    }

    return min;
}

rbtree_node_t* rbtree_node_successor(rbtree_node_t *node) {
    if (node == NULL) return NULL;

    // The successor of a node is the min of it's right subtree (i.e. the right subtree contains all nodes greater
    // than it so we must find the smallest value in that subtree)
    rbtree_node_t *right = rbtree_right(node);
    if (right != NULL) {
        return rbtree_node_min(right);
    } else {
        // If a right subtree does not exist then the successor must be one of it's ancestors
        // Specifically it's one of the parent nodes where this node is found in it's left subtree
        rbtree_node_t *parent;

        for (parent = rbtree_parent(node); parent != NULL; parent = rbtree_parent(parent)) {
            if (node == rbtree_left(parent)) return parent;
            node = parent;
        }

        return NULL;
    }
}

rbtree_node_t* rbtree_node_predecessor(rbtree_node_t *node) {
    if (node == NULL) return NULL;

    // The predecessor of a node is the max of it's left subtree (i.e. the left subtree contains all nodes lesser than
    // it so we must find the largest value in that subtree)
    rbtree_node_t *left = rbtree_left(node);
    if (left != NULL) {
        return rbtree_node_max(left);
    } else {
        // If a left subtree does not exist then the successor must be one of it's ancestors
        // Specifically it's one of the parent nodes where this node is found in it's right subtree
        rbtree_node_t *parent;

        for (parent = rbtree_parent(node); parent != NULL; parent = rbtree_parent(parent)) {
            if (node == rbtree_right(parent)) return parent;
            node = parent;
        }

        return NULL;
    }
}

bool rbtree_insert_here(rbtree_t *tree, rbtree_node_t *parent, rbtree_child_t child, rbtree_node_t *node) {
    if (tree == NULL || node == NULL) return false;

    // Make sure node isn't already part of a tree
    if (rbtree_parent(node) != NULL || rbtree_left(node) != NULL || rbtree_right(node) != NULL) return false;

    // We can't have this condition, if parent is NULL then we are inserting in the root so root must be NULL
    if (parent == NULL && rbtree_root(tree) != NULL) return false;

    rbtree_node_init(node);
    rbtree_set_parent(node, parent);

    if (parent == NULL) {
        rbtree_root(tree) = node;
    } else {
        rbtree_this_child(parent, child) = node;
    }

    while (true) {
        if (parent == NULL) {
            rbtree_set_black(node);
            break;
        }

        if (rbtree_is_black(parent)) break;

        rbtree_node_t *grandparent = rbtree_parent(parent);
        if (grandparent == NULL) return false;

        rbtree_node_t *uncle = rbtree_sibling(parent);
        rbtree_dir_t dir = rbtree_left(grandparent) == parent ? RBTREE_DIR_LEFT : RBTREE_DIR_RIGHT;

        // Uncle is red, swap colours and restart the loop at the grandparent
        if (rbtree_is_red(uncle)) {
            rbtree_set_black(uncle);
            rbtree_set_black(parent);
            rbtree_set_red(grandparent);
            node = grandparent;
            parent = rbtree_parent(node);
            continue;
        }

        // Node is on the inner tree of parent. Rotate it so it moves to parent's position
        if (node == rbtree_this_child(parent, dir ^ 1)) {
            _rbtree_rotate(tree, parent, dir);
            rbtree_node_t *tmp = node;
            node = parent;
            parent = tmp;
        }

        // Node is on the outer tree of parent. Update colours and rotate it so it moves to parent's position
        rbtree_set_black(parent);
        rbtree_set_red(grandparent);
        _rbtree_rotate(tree, grandparent, dir ^ 1);
        break;
    }

    return rbtree_is_black(rbtree_root(tree));
}

bool rbtree_insert_slot(rbtree_t *tree, rbtree_slot_t slot, rbtree_node_t *node) {
    rbtree_node_t *parent = rbtree_slot_extract_parent(slot);
    rbtree_child_t child = rbtree_slot_extract_child(slot);
    return rbtree_insert_here(tree, parent, child, node);
}

bool rbtree_insert(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *node) {
    rbtree_node_t *parent = NULL;
    rbtree_child_t child;

    // Walk the tree going left if node is less than and going right if it's greater than the current node we are
    // examining
    for (rbtree_node_t *here = rbtree_root(tree); here != NULL; here = rbtree_this_child(parent, child)) {
        rbtree_compare_result_t cmp = compare_func(node, here);

        // The node we are inserting shouldn't already exist in our tree
        if (cmp == RBTREE_COMPARE_EQ) return false;

        parent = here;
        child = rbtree_which_child(cmp);
    }

    return rbtree_insert_here(tree, parent, child, node);
}

bool rbtree_remove(rbtree_t *tree, rbtree_node_t *node) {
    if (node == NULL) return false;

    // Normal binary tree delete: get the successor if one exists and swap places with the node to delete
    // But only if the node has both non-null children
    rbtree_node_t *successor = rbtree_left(node) == NULL || rbtree_right(node) == NULL ? node
        : rbtree_node_successor(node);

    rbtree_node_t *child = rbtree_left(successor) != NULL ? rbtree_left(successor) : rbtree_right(successor);
    rbtree_node_t *parent = rbtree_parent(successor);
    int colour = rbtree_get_colour(successor);

    // Replace the successor with it's child
    if (child != NULL) rbtree_set_parent(child, parent);

    if (parent == NULL) {
        rbtree_root(tree) = child;
    } else {
        rbtree_child_t which = successor == rbtree_left(parent) ? RBTREE_CHILD_LEFT : RBTREE_CHILD_RIGHT;
        rbtree_this_child(parent, which) = child;
    }

    // Replace the deleted node with the successor
    if (node != successor) {
        rbtree_node_t *p = rbtree_parent(node), *l = rbtree_left(node), *r = rbtree_right(node);

        if (p == NULL) {
            rbtree_root(tree) = successor;
        } else {
            rbtree_child_t which = rbtree_left(p) == node ? RBTREE_CHILD_LEFT : RBTREE_CHILD_RIGHT;
            rbtree_this_child(p, which) = successor;
        }

        if (l != NULL) rbtree_set_parent(l, successor);
        if (r != NULL) rbtree_set_parent(r, successor);


        *successor = *node;
    }

    rbtree_node_init(node);

    if (colour == RBTREE_NODE_RED) return true;

    if (node == parent) parent = successor;
    node = child;

    while (true) {
        if (rbtree_is_red(node)) {
            rbtree_set_black(node);
            break;
        }

        if (parent == NULL) break;

        rbtree_dir_t dir = rbtree_left(parent) == node ? RBTREE_DIR_LEFT : RBTREE_DIR_RIGHT;
        rbtree_node_t *sibling = rbtree_this_child(parent, dir ^ 1);

        // Sibling is red. Update colour and rotate at the parent so that sibling is black
        if (rbtree_is_red(sibling)) {
            rbtree_set_black(sibling);
            rbtree_set_red(parent);
            _rbtree_rotate(tree, parent, dir);
            sibling = rbtree_this_child(parent, dir ^ 1);
        }

        // Sibling only has black children. Update colours and rerun loop at parent
        if (rbtree_is_black(rbtree_left(sibling)) && rbtree_is_black(rbtree_right(sibling))) {
            rbtree_set_red(sibling);
            node = parent;
            parent = rbtree_parent(node);
            continue;
        }

        // Sibling's outer child is black. Update colour and rotate that way to move red child to sibling's position
        if (rbtree_is_black(rbtree_this_child(sibling, dir ^ 1))) {
            rbtree_set_black(rbtree_this_child(sibling, dir));
            rbtree_set_red(sibling);
            _rbtree_rotate(tree, sibling, dir ^ 1);
            sibling = rbtree_this_child(parent, dir ^ 1);
        }

        // Sibling's inner child is black. Swap sibling and parent colours, set the sibling's other child black
        // and rotate so that the inner child is at sibling's position
        rbtree_set_colour(sibling, rbtree_get_colour(parent));
        rbtree_set_black(parent);
        rbtree_set_black(rbtree_this_child(sibling, dir ^ 1));
        _rbtree_rotate(tree, parent, dir);
        break;
    }

    return rbtree_is_black(rbtree_root(tree));
}

void rbtree_clear(rbtree_t *tree, rbtree_delete_func_t delete_func) {
    rbtree_node_t *parent;

    for (rbtree_node_t *node = _rbtree_deepest(rbtree_root(tree)); node != NULL; node = _rbtree_deepest(parent)) {
        parent = _rbtree_emancipate(node);
        delete_func(node);
    }
}

rbtree_node_t* rbtree_search_slot(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *key,
    rbtree_slot_t *slot) {
    rbtree_node_t *here = rbtree_root(tree), *parent = NULL;
    rbtree_child_t child;

    for (rbtree_compare_result_t cmp; here != NULL; here = rbtree_this_child(here, child)) {
        cmp = compare_func(key, here);

        // Found a match
        if (cmp == RBTREE_COMPARE_EQ) break;

        parent = here;
        child = rbtree_which_child(cmp);
    }

    // Set the slot in the tree where the key may be inserted
    if (slot != NULL) *slot = rbtree_slot_init(parent, child);

    return here;
}

rbtree_node_t* rbtree_search(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *key) {
    rbtree_slot_t slot;
    return rbtree_search_slot(tree, compare_func, key, &slot);
}

bool rbtree_search_nearest(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *key,
    rbtree_node_t **nearest, rbtree_slot_t *slot, rbtree_dir_t dir) {
    rbtree_node_t *here = rbtree_root(tree), *parent = NULL;
    rbtree_child_t child;
    rbtree_compare_result_t cmp;

    // Empty tree
    if (here == NULL) return NULL;

    for (; here != NULL; here = rbtree_this_child(here, child)) {
        cmp = compare_func(key, here);

        // Found a match
        if (cmp == RBTREE_COMPARE_EQ) break;

        parent = here;
        child = rbtree_which_child(cmp);
    }

    // If we found a match just return the matching node and it's slot
    if (slot != NULL) *slot = rbtree_slot_init(parent, child);

    if (cmp != RBTREE_COMPARE_EQ) {
        // If we didn't find a match temporarily insert a dummy node in the slot (this is where the key node
        // would be in a binary search tree) and find the predecessor node to it
        rbtree_node_t node = RBTREE_NODE_INITIALIZER, *nodep = &node;

        rbtree_this_child(parent, child) = nodep;
        rbtree_set_parent(nodep, parent);
        here = (dir == RBTREE_DIR_SUCCESSOR) ? rbtree_node_successor(nodep) : rbtree_node_predecessor(nodep);

        // Now remove the temporary node
        _rbtree_emancipate(nodep);
    }

    *nearest = here;
    return (cmp == RBTREE_COMPARE_EQ);
}

bool rbtree_search_predecessor(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *key,
    rbtree_node_t **predecessor, rbtree_slot_t *slot) {
    return rbtree_search_nearest(tree, compare_func, key, predecessor, slot, RBTREE_DIR_PREDECESSOR);
}

bool rbtree_search_successor(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *key,
    rbtree_node_t **successor, rbtree_slot_t *slot) {
    return rbtree_search_nearest(tree, compare_func, key, successor, slot, RBTREE_DIR_SUCCESSOR);
}


void rbtree_node_walk_inorder(rbtree_node_t *node, rbtree_walk_func_t walk) {
    if (node == NULL) return;

    rbtree_node_t *left = rbtree_left(node), *right = rbtree_right(node);

    if (left != NULL) rbtree_node_walk_inorder(left, walk);
    walk(node);
    if (right != NULL) rbtree_node_walk_inorder(right, walk);
}

#if DEBUG
extern void kprintf(const char *fmt, ...);

typedef struct trunk_s
{
    struct trunk_s *prev;
    char *str;
} trunk_t;

// Helper function to print branches of the binary tree
void _rbtree_show_trunks(trunk_t *p) {
    if (p == NULL) return;

    _rbtree_show_trunks(p->prev);

    kprintf("%s", p->str);
}

// Recursive function to print binary tree
// It uses inorder traversal
void _rbtree_print_recurse(rbtree_node_t *node, trunk_t *prev, bool is_right, rbtree_walk_func_t print_func) {
    if (node == NULL)
        return;

    char *prev_str = "    ";
    trunk_t trunk = { .prev = prev, .str = prev_str };

    _rbtree_print_recurse(rbtree_right(node), &trunk, true, print_func);

    if (!prev)
        trunk.str = "---";
    else if (is_right)
    {
        trunk.str = ",---";
        prev_str = "   |";
    }
    else
    {
        trunk.str = "`---";
        prev->str = prev_str;
    }

    _rbtree_show_trunks(&trunk);
    print_func(node);

    if (prev) prev->str = prev_str;
    trunk.str = "   |";

    _rbtree_print_recurse(rbtree_left(node), &trunk, false, print_func);
}

void _rbtree_print(rbtree_t *tree, rbtree_walk_func_t print_func) {
    if (tree != NULL) _rbtree_print_recurse(rbtree_root(tree), NULL, false, print_func);
}
#endif // DEBUG
