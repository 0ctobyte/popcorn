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

#define structof(ptr, struct_type, struct_member) ((struct_type*)((uintptr_t)(ptr) - offsetof(struct_type, struct_member)))

#define list_entry(list, struct_type, struct_member) (structof(list, struct_type, struct_member))

#define list_first(list)    ((list)->first)
#define list_last(list)     ((list)->last)
#define list_next(node)     ((node)->next)
#define list_prev(node)     ((node)->prev)
#define list_end(node)      ((node) == NULL)
#define list_is_empty(list) ((list)->first == NULL)

void list_init(list_t *list);
void list_node_init(list_node_t *list_node);
size_t list_count(list_t *list);

bool list_insert(list_t *list, list_node_t *prev, list_node_t *next, list_node_t *node);
bool list_insert_after(list_t *list, list_node_t *prev, list_node_t *node);
bool list_insert_before(list_t *list, list_node_t *next, list_node_t *node);
bool list_insert_last(list_t *list, list_node_t *node);
bool list_insert_first(list_t *list, list_node_t *node);

bool list_remove(list_t *list, list_node_t *node);

#define list_clear(list, delete_func, struct_type, struct_member)\
{\
    while (!list_is_empty(list)) {\
        list_remove(list, list_first(list));\
        delete_func(list_entry(list_first(list), struct_type, struct_member));\
    }\
}\

// Lookup an element in the list using the compare_func and key. Key can be any data type
// and the comparator_func must take as arguments the key and list entry. It shall return true
// if the list entry and key match (in w/e definition of the word "match" is employed) and false otherwise
#define list_lookup(list, key, compare_func, struct_type, struct_member)\
{\
    list_node_t *node;\
    for (node = list_first(list); !list_end(node); node = list_next(node)) {\
        if (compare_func(list_entry(node, struct_type, struct_member), key)) break;\
    }\
    node;\
}\

#define list_for_each_entry(list, entry, struct_member)\
    for (entry = list_entry(list_first(list), typeof(*entry), struct_member);\
        !list_end(&entry->struct_member);\
        entry = list_entry(list_next(&entry->struct_member), typeof(*entry), struct_member))\

#endif // __LIST_H__
