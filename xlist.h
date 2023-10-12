
#ifndef X_LIST_H
#define X_LIST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct node_t node_t;

typedef struct xlist_t xlist_t;

// create a list instance
xlist_t *xlist_create();

// destroy the xlist instance
void xlist_destroy(xlist_t *_list);

// return the first element
// and reset the iterator to head
node_t *xlist_begin(xlist_t *_list);

// return the next element
// and move the iterator to next
node_t *xlist_next(xlist_t *_list);

// return the number of elements
size_t xlist_count(xlist_t *_list);

// insert the new element to head
int xlist_insert(xlist_t *_list, node_t *_node);

// append the new element to tail
int xlist_append(xlist_t *_list, node_t *_node);

// remove the first element and return it
node_t *xlist_remove_head(xlist_t *_list);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !X_LIST_H

