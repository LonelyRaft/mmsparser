
#include "node.h"
#include <stdlib.h>

int node_destroy(node_t* _node)
{
    if(_node == NULL){
        return 0;
    }
    if(_node->op == NULL){
        free(_node);
        _node = NULL;
        return 0;
    }
    if(_node->op->destroy == NULL){
        return 0;
    }
    return _node->op->destroy(_node);
}
