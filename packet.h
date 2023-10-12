
#ifndef PACKET_MMS_H
#define PACKET_MMS_H

#include "xlist.h"
#include "xvalue.h"
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define NODE_TYPE_INVALID (0)
#define NODE_TYPE_FILESPEC (1)
#define NODE_TYPE_DIRENTRY (2)
#define NODE_TYPE_VARSPEC (3)
#define NODE_TYPE_UDATA (4)


node_t *node_create(int _type);

int node_destroy(node_t *_node);

node_t *file_spec_create();

size_t file_spec_length(const node_t *_node);

const char *file_spec_path(
        node_t *_node, const char *_path,
        unsigned int _length);

node_t *dir_entry_create();

const char *dir_entry_path(
        node_t *_node, const char *_path,
        unsigned int _length);

size_t dir_entry_path_len(node_t *_node);

size_t dir_entry_size(node_t *_node, unsigned int *_size);

const struct tm *dir_entry_stamp(
        node_t *_node, const char *_stamp);

node_t *var_spec_create();

const char *var_spec_domain(
        node_t *_node, const char *_domain,
        unsigned int _length);

size_t var_spec_domain_len(node_t *_node);

const char *var_spec_index(
        node_t *_node, const char *_index,
        unsigned int _length);

size_t var_spec_index_len(node_t *_node);

node_t *udata_create();

const xvalue_t *udata_value(
        node_t *_node, const xvalue_t *_value);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !PACKET_MMS_H
