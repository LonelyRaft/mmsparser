
#ifndef NODE_H
#define NODE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct node_t node_t;

typedef struct node_op_t {
    int (*destroy)(node_t *);

    int (*tostring)(node_t *, char *, size_t);
} node_op_t;

// abstract node type
typedef struct node_t {
    int type;
    struct node_t *next;
    const node_op_t *op;
} node_t;

int node_destroy(node_t *_node);

int node_tostring(
        node_t *_node,
        char *_dest, size_t _size);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !NODE_H
