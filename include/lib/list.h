#ifndef __LIST_H__
#define __LIST_H__

#include <sys/types.h>

typedef struct list_node_s {
    struct list_node_s *next;
    struct list_node_s *prev;
} list_node_t;

typedef struct {
    list_node_t *first;
    list_node_t *last;
} list_t;

typedef enum {
    LIST_COMPARE_LT,
    LIST_COMPARE_EQ,
    LIST_COMPARE_GT,
} list_compare_result_t;

typedef enum {
    LIST_ORDER_ASCENDING,
    LIST_ORDER_DESCENDING,
} list_order_t;

// Callback function to free any resources of the given node
typedef void (*list_delete_func_t)(list_node_t*);

// Callback function to compare a node with the provided key (which could be any kind of data)
typedef list_compare_result_t (*list_compare_func_t)(list_node_t*,list_node_t*);

#define structof(ptr, struct_type, struct_member)    ((struct_type*)((uintptr_t)(ptr) - offsetof(struct_type, struct_member)))
#define list_entry(node, struct_type, struct_member) ((node) != NULL ? (structof(node, struct_type, struct_member)) : NULL)

#define list_first(list)      ((list)->first)
#define list_last(list)       ((list)->last)
#define list_next(node)       ((node)->next)
#define list_prev(node)       ((node)->prev)
#define list_end(node)        ((node) == NULL)
#define list_is_empty(list)   ((list)->first == NULL)

#define list_push(list, node) (list_insert_first(list, node))
#define list_pop(list, node)  ({ (node) = list_first(list); list_remove(list, node); })

#define LIST_INITIALIZER      (list_t){ .first = NULL, .last = NULL}
#define LIST_NODE_INITIALIZER (list_node_t){ .next = NULL, .prev = NULL}

void list_init(list_t *list);
void list_node_init(list_node_t *list_node);
size_t list_count(list_t *list);

bool list_insert_here(list_t *list, list_node_t *prev, list_node_t *next, list_node_t *node);

#define list_insert_after(list, prev, node)  (list_insert_here(list, prev, ((prev) == NULL) ? (list)->first : (prev)->next, node))
#define list_insert_before(list, next, node) (list_insert_here(list, ((next) == NULL) ? (list)->last : (next)->prev, next, node))
#define list_insert_last(list, node)         (list_insert_here(list, (list)->last, NULL, node))
#define list_insert_first(list, node)        (list_insert_here(list, NULL, (list)->first, node))

bool list_insert(list_t *list, list_compare_func_t compare_func, list_node_t *node, list_order_t order);

bool list_remove(list_t *list, list_node_t *node);
bool list_clear(list_t *list, list_delete_func_t delete_func);

// Search for an element in the list using the compare_func and key.
// The compare_func must take as arguments the key and list node to compare to. It shall return LIST_COMPARE_EQ
// if the list entry and key match (in w/e definition of the word "match" is employed) and either LIST_COMPARE_GT or LIST_COMPARE_LT  otherwise
// This routine will return a valid node if one is found otherwise it shall return NULL
list_node_t* list_search(list_t *list, list_compare_func_t compare_func, list_node_t *key);

#define list_for_each_entry(list, entry, struct_member)\
    for (entry = list_entry(list_first(list), typeof(*entry), struct_member);\
        !list_end(&entry->struct_member);\
        entry = list_entry(list_next(&entry->struct_member), typeof(*entry), struct_member))

#endif // __LIST_H__
