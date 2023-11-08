
#include "node.h"

#include <stdlib.h>

int node_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->op == NULL) {
        free(_node);
        _node = NULL;
        return 0;
    }
    if (_node->op->destroy == NULL) {
        // log warn
        return 0;
    }
    return _node->op->destroy(_node);
}

int node_tostring(
        node_t *_node,
        char *_dest, size_t _size) {
    if (_node == NULL || _dest == NULL) {
        return 0;
    }
    if (_node->op == NULL ||
        _node->op->tostring == NULL) {
        return 0;
    }
    return _node->op->tostring(
            _node, _dest, _size);
}
