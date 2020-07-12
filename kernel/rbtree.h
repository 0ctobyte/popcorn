#ifndef _RBTREE_H_
#define _RBTREE_H_

#include <sys/types.h>

// Assuming nodes are at least 2-byte aligned. If they are, we can embed the colour information in the pointer
#define RBTREE_NODE_BLACK (0)
#define RBTREE_NODE_RED   (1)

typedef enum {
    RBTREE_CHILD_LEFT  = 0,
    RBTREE_CHILD_RIGHT = 1,
} rbtree_child_t;

typedef struct rbtree_node_s {
    struct rbtree_node_s *parent;
    struct rbtree_node_s *children[2];
} rbtree_node_t;

typedef struct {
    rbtree_node_t *root;
} rbtree_t;

// A rbtree slot is just an encoding of a parent node and one of it's children where a given node may be inserted
// The child index (i.e. left or right) is encoded in the lsb of the parent node's pointer (assumption is all nodes
// are at least 2-byte aligned)
typedef rbtree_node_t* rbtree_slot_t;

// Direction for nearest search function
typedef enum {
    RBTREE_DIR_LEFT = 0,
    RBTREE_DIR_RIGHT = 1,
    RBTREE_DIR_SUCCESSOR = 0,
    RBTREE_DIR_PREDECESSOR = 1,
} rbtree_dir_t;

// Return value from compare functions
typedef enum {
    RBTREE_COMPARE_LT,
    RBTREE_COMPARE_EQ,
    RBTREE_COMPARE_GT,
} rbtree_compare_result_t;

typedef rbtree_compare_result_t (*rbtree_compare_func_t)(rbtree_node_t*,rbtree_node_t*);
typedef void (*rbtree_delete_func_t)(rbtree_node_t*);
typedef void (*rbtree_walk_func_t)(rbtree_node_t*);

#define structof(ptr, struct_type, struct_member)      ((struct_type*)((uintptr_t)(ptr) -\
    offsetof(struct_type, struct_member)))
#define rbtree_entry(node, struct_type, struct_member) ((node) != NULL ? (structof(node, struct_type, struct_member))\
    : NULL)

#define rbtree_root(tree)                              ((tree)->root)
#define rbtree_is_empty(tree)                          (rbtree_root(tree) == NULL)
#define rbtree_make_colour(node, colour)               ((rbtree_node_t*)(((uintptr_t)(node)->parent &\
    ~RBTREE_NODE_RED) | ((colour) & 0x1)))
#define rbtree_make_black(node)                        ((rbtree_node_t*)((uintptr_t)(node)->parent & ~RBTREE_NODE_RED))
#define rbtree_make_red(node)                          ((rbtree_node_t*)((uintptr_t)(node)->parent | RBTREE_NODE_RED))
#define rbtree_set_colour(node, colour)                ((node)->parent = rbtree_make_colour(node, colour));
#define rbtree_set_black(node)                         ((node)->parent = rbtree_make_black(node))
#define rbtree_set_red(node)                           ((node)->parent = rbtree_make_red(node))
#define rbtree_get_colour(node)                        ((uintptr_t)(node)->parent & RBTREE_NODE_RED)
#define rbtree_is_black(node)                          (rbtree_get_colour(node) == RBTREE_NODE_BLACK)
#define rbtree_is_red(node)                            (rbtree_get_colour(node) == RBTREE_NODE_RED)
#define rbtree_make_parent(child, new_parent)          ((rbtree_node_t*)((uintptr_t)(new_parent) |\
    ((uintptr_t)(child)->parent & RBTREE_NODE_RED)))
#define rbtree_set_parent(child, new_parent)           ((child)->parent = rbtree_make_parent(child, new_parent))
#define rbtree_which_child(cmp)                        ((cmp) == RBTREE_COMPARE_LT ? RBTREE_CHILD_LEFT :\
    RBTREE_CHILD_RIGHT)
#define rbtree_this_child(parent, child)               ((parent)->children[child])
#define rbtree_slot_init(parent, child)                ((rbtree_slot_t)((uintptr_t)(parent) |\
    ((unsigned long)(child) & 0x1)))
#define rbtree_slot_extract_parent(slot)               ((rbtree_node_t*)((uintptr_t)(slot) & ~0x1))
#define rbtree_slot_extract_child(slot)                ((rbtree_child_t)((uintptr_t)(slot) & 0x1))

#define rbtree_min(tree)                               (rbtree_node_min(rbtree_root(tree)))
#define rbtree_max(tree)                               (rbtree_node_max(rbtree_root(tree)))
#define rbtree_successor(tree)                         (rbtree_node_successor(rbtree_root(tree)))
#define rbtree_predecessor(tree)                       (rbtree_node_predecessor(rbtree_root(tree)))
#define rbtree_walk_inorder(tree, func)                (rbtree_node_walk_inorder(rbtree_root(tree), func))
#define rbtree_parent(node)                            ((rbtree_node_t*)((uintptr_t)(node)->parent & ~RBTREE_NODE_RED))
#define rbtree_left(node)                              ((node)->children[RBTREE_CHILD_LEFT])
#define rbtree_right(node)                             ((node)->children[RBTREE_CHILD_RIGHT])

#define RBTREE_INITIALIZER                             ((rbtree_t){ .root = NULL })
#define RBTREE_NODE_INITIALIZER                        ((rbtree_node_t){ .parent = (rbtree_node_t*)(RBTREE_NODE_RED),\
    .children = {NULL, NULL} })

#define rbtree_init(tree)                              (*(tree) = RBTREE_INITIALIZER)
#define rbtree_node_init(node)                         (*(node) = RBTREE_NODE_INITIALIZER)

rbtree_node_t* rbtree_grandparent(rbtree_node_t *node);
rbtree_node_t* rbtree_sibling(rbtree_node_t *node);
rbtree_node_t* rbtree_uncle(rbtree_node_t *node);

// Finds the min/max value in the tree/subtree rooted at the given node
rbtree_node_t* rbtree_node_max(rbtree_node_t *node);
rbtree_node_t* rbtree_node_min(rbtree_node_t *node);

// Finds the successor (the next greatest node) and predecessor (the next lesser node) of the given node
rbtree_node_t* rbtree_node_successor(rbtree_node_t *node);
rbtree_node_t* rbtree_node_predecessor(rbtree_node_t *node);

// In-order tree walk function from the given node, calls the given function for each node visited
void rbtree_node_walk_inorder(rbtree_node_t *node, rbtree_walk_func_t walk);

// Inserts under the given parent as the specified child (left or right) and rebalances the tree
bool rbtree_insert_here(rbtree_t *tree, rbtree_node_t *parent, rbtree_child_t child, rbtree_node_t *node);
bool rbtree_insert_slot(rbtree_t *tree, rbtree_slot_t slot, rbtree_node_t *node);

// Walks the tree to find the appropriate location to insert the node
// This function takes a compare_func which is given two rbtree_node_t structs
// and shall return whether the first argument is less then the second, equal to, or greater than
bool rbtree_insert(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *node);

// Remove the node from the tree or clear the entire tree freeing the nodes resources
bool rbtree_remove(rbtree_t *tree, rbtree_node_t *node);
void rbtree_clear(rbtree_t *tree, rbtree_delete_func_t delete_func);

// Searches for the given key, in either the found or not found case, slot will hold the parent and child position that
// the given key may be inserted at. This saves an extra tree walk if key will/will not be inserted as a result of this
// search
rbtree_node_t* rbtree_search_slot(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *key,
    rbtree_slot_t *slot);

// Search for the node that matches the given key (in w/e sense of the word "matches" according to the compare_func)
rbtree_node_t* rbtree_search(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *key);

// Searches for either the given node with the matching key or the predecessor or successor depending on the dir
// argument. Returns true if a perfect match was found else false It shall return the matching/predecessor/successor
// node (or NULL if the tree is empty) as well as a rbtree_slot_t which encodes the parent and child position a new
// node with the provided key may be inserted. This saves a second tree walk if a new node with the given key needs to
// be inserted as a result of this search
bool rbtree_search_nearest(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *key,
    rbtree_node_t **nearest, rbtree_slot_t *slot, rbtree_dir_t dir);

// Searches for the largest node less than or equal to the given key (i.e. the predecessor to the given key if it were
// a node). Returns true if a match was found otherwise false. In either case the matching node or predecessor is
// stored in predecessor
bool rbtree_search_predecessor(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *key,
    rbtree_node_t **predecessor, rbtree_slot_t *slot);

// Searches for the smallest node greater than or equal to the given key (i.e. the successor to the given key if it
// were a node). Returns true if a match was found otherwise false. In either case the matching node or successor is
// stored in successor
bool rbtree_search_successor(rbtree_t *tree, rbtree_compare_func_t compare_func, rbtree_node_t *key,
    rbtree_node_t **successor, rbtree_slot_t *slot);

#endif // _RBTREE_H_
