
#include "xlist.h"

#include <stdlib.h>

#include "node.h"

typedef struct xlist_t {
  node_t *head;
  node_t *tail;
  node_t *it;
  size_t count;
} xlist_t;

// create a list instance
xlist_t *xlist_create() {
  const size_t size = sizeof(xlist_t) + sizeof(node_t);
  xlist_t *list = (xlist_t *)malloc(size);
  if (list == NULL) {
    return list;
  }
  list->head = (node_t *)(list + 1);
  list->head->next = NULL;
  list->tail = list->head;
  list->it = list->head;
  list->count = 0;
  return list;
}

// destroy the xlist instance
void xlist_destroy(xlist_t *_list) {
  if (_list == NULL) {
    return;
  }
  node_t *it = xlist_remove_head(_list);
  while (it != NULL) {
    node_destroy(it);
    it = xlist_remove_head(_list);
  }
  free(_list);
  _list = NULL;
}

// return the first element
// and reset the iterator to head
node_t *xlist_begin(xlist_t *_list) {
  if (_list == NULL) {
    return NULL;
  }
  _list->it = _list->head;
  return _list->it->next;
}

// return the next element
// and move the iterator to next
node_t *xlist_next(xlist_t *_list) {
  if (_list == NULL) {
    return NULL;
  }
  if (_list->it == NULL) {
    return NULL;
  }
  _list->it = _list->it->next;
  return _list->it->next;
}

// return the number of elements
size_t xlist_count(xlist_t *_list) {
  if (_list == NULL) {
    return 0;
  }
  return _list->count;
}

// append the new element to tail
int xlist_append(xlist_t *_list, node_t *_node) {
  if (_list == NULL || _node == NULL) {
    return -1;
  }
  _list->tail->next = _node;

  _list->tail = _list->tail->next;
  _list->count++;
  return 0;
}

// insert the new element to head
int xlist_insert(xlist_t *_list, node_t *_node) {
  if (_list == NULL || _node == NULL) {
    return -1;
  }
  _node->next = _list->head->next;
  _list->head->next = _node;

  _list->count++;
  return 0;
}

// remove the first element and return it
node_t *xlist_remove_head(xlist_t *_list) {
  if (_list == NULL) {
    return NULL;
  }
  node_t *node = _list->head->next;
  if (node != NULL) {
    _list->head->next = node->next;
    _list->count--;
  }
  return node;
}
