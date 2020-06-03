#include <lib/list.h>

void list_init(list_t *list) {
    list->first = NULL;
    list->last = NULL;
}

void list_node_init(list_node_t *node) {
    node->next = NULL;
    node->prev = NULL;
}

size_t list_count(list_t *list) {
    size_t count = 0;
    for (list_node_t *node = list_first(list); !list_end(node); node = list_next(node)) {
        count++;
    }
    return count;
}

bool list_insert(list_t *list, list_node_t *prev, list_node_t *next, list_node_t *node) {
    if (list == NULL || node == NULL) return false;

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

bool list_insert_after(list_t *list, list_node_t *prev, list_node_t *node) {
    list_node_t *next = prev == NULL ? list->first : prev->next;
    return list_insert(list, prev, next, node);
}

bool list_insert_before(list_t *list, list_node_t *next, list_node_t *node) {
    list_node_t *prev = next == NULL ? list->last : next->prev;
    return list_insert(list, prev, next, node);
}

bool list_insert_last(list_t *list, list_node_t *node) {
    return list_insert(list, list->last, NULL, node);
}

bool list_insert_first(list_t *list, list_node_t *node) {
    return list_insert(list, NULL, list->first, node);
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
