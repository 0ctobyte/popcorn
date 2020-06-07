#include <lib/list.h>

void list_init(list_t *list) {
    *list = LIST_INITIALIZER;
}

void list_node_init(list_node_t *node) {
    *node = LIST_NODE_INITIALIZER;
}

size_t list_count(list_t *list) {
    size_t count = 0;
    for (list_node_t *node = list_first(list); !list_end(node); node = list_next(node)) {
        count++;
    }
    return count;
}

bool list_insert_here(list_t *list, list_node_t *prev, list_node_t *next, list_node_t *node) {
    if (list == NULL || node == NULL) return false;

    // Make sure node isn't already linked elsewhere
    if (node->prev != NULL || node->next != NULL) return false;

    node->prev = prev;
    if (prev == NULL) {
        list->first = node;
    } else {
        prev->next = node;
    }

    node->next = next;
    if (next == NULL) {
        list->last = node;
    } else {
        next->prev = node;
    }

    return true;
}

bool list_insert(list_t *list, list_compare_func_t compare_func, list_node_t *node, list_order_t order) {
    if (list == NULL || node == NULL) return false;

    list_node_t *here = list_first(list);

    // Search for position to insert node
    for (; here != NULL; here = list_next(here)) {
        list_compare_result_t cmp = compare_func(node, here);

        // It's up to the compare_func to decide whether the list supports non-unique list elements; either return LT or GT if the elements are equal
        // Otherwise we assume list elements are unique and fail if we are trying to insert an element that already exists in the list
        if (cmp == LIST_COMPARE_EQ) return false;

        // Insert before here if order is ascending and node < here or if order is descending and node > here
        if ((order == LIST_ORDER_ASCENDING && cmp == LIST_COMPARE_LT) || (order == LIST_ORDER_DESCENDING && cmp == LIST_COMPARE_GT)) {
            break;
        }
    }

    return list_insert_before(list, here, node);
}

bool list_remove(list_t *list, list_node_t *node) {
    if (list == NULL || node == NULL) return false;

    list_node_t *prev = node->prev;
    list_node_t *next = node->next;

    if (prev == NULL) {
        list->first = next;
    } else {
        prev->next = next;
    }

    if (next == NULL) {
        list->last = prev;
    } else {
        next->prev = prev;
    }

    list_node_init(node);
    return true;
}

bool list_clear(list_t *list, list_delete_func_t delete_func) {
    while (!list_is_empty(list)) {
        list_node_t *node = list_first(list);
        if (!list_remove(list, node)) return false;
        delete_func(node);
    }

    return true;
}

list_node_t* list_search(list_t *list, list_compare_func_t compare_func, list_node_t *key) {
    list_node_t *node = NULL;

    for (node = list_first(list); !list_end(node); node = list_next(node)) {
        if (compare_func(node, key)) break;
    }

    return node;
}
