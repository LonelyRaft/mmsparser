

#include "packet.h"
#include "node.h"
#include <limits.h>
#include <string.h>
#include <stdlib.h>

typedef struct file_spec_t {
    node_t parent;
    mmsstr_t path;
} file_spec_t;

static int file_spec_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_FILESPEC) {
        return -1;
    }
    file_spec_t *file = (file_spec_t *) _node;
    if (file->path.length >= STRING_SHORT_SIZE) {
        free(file->path.data.long_str);
        file->path.data.long_str = NULL;
    }
    free(_node);
    return 0;
}

node_t *file_spec_create() {
    static const node_op_t nodeop = {
            file_spec_destroy,
    };
    node_t *node = (node_t *) malloc(sizeof(file_spec_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(file_spec_t));
    node->type = NODE_TYPE_FILESPEC;
    node->op = &nodeop;
    return node;
}

size_t file_spec_length(const node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_FILESPEC) {
        return 0;
    }
    file_spec_t *file = (file_spec_t *) _node;
    return file->path.length;
}

const char *file_spec_path(
        node_t *_node, const char *_path,
        unsigned int _length) {
    if (_node == NULL) {
        return NULL;
    }
    if (_node->type != NODE_TYPE_FILESPEC) {
        return NULL;
    }
    file_spec_t *file = (file_spec_t *) _node;
    if (_path == NULL) {
        return mmsstr_data(&file->path);
    }
    return mmsstr_set_data(&file->path, _path, _length);
}

typedef struct file_attr_t {
    size_t size;
    struct tm stamp;
} file_attr_t;

typedef struct dir_entry_t {
    node_t parent;
    mmsstr_t name;
    file_attr_t attr;
} dir_entry_t;

static int dir_entry_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_DIRENTRY) {
        return -1;
    }
    dir_entry_t *entry = (dir_entry_t *) _node;
    if (entry->name.length >= STRING_SHORT_SIZE) {
        free(entry->name.data.long_str);
        entry->name.data.long_str = NULL;
        entry->name.length = 0;
    }
    free(entry);
    entry = NULL;
    return 0;
}

node_t *dir_entry_create() {
    static const node_op_t nodeop = {
            dir_entry_destroy,
    };
    node_t *node = (node_t *) malloc(sizeof(dir_entry_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(dir_entry_t));
    node->type = NODE_TYPE_DIRENTRY;
    node->op = &nodeop;
    return node;
}

const char *dir_entry_path(
        node_t *_node, const char *_path,
        unsigned int _length) {
    if (_node == NULL) {
        return NULL;
    }
    if (_node->type != NODE_TYPE_DIRENTRY) {
        return NULL;
    }
    dir_entry_t *entry = (dir_entry_t *) _node;
    if (_path == NULL) {
        return mmsstr_data(&entry->name);
    }
    return mmsstr_set_data(&entry->name, _path, _length);
}

size_t dir_entry_path_len(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_DIRENTRY) {
        return 0;
    }
    return ((dir_entry_t *) _node)->name.length;
}

size_t dir_entry_size(node_t *_node, unsigned int *_size) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_DIRENTRY) {
        return 0;
    }
    dir_entry_t *entry = (dir_entry_t *) _node;
    if (_size == NULL) {
        return entry->attr.size;
    }
    entry->attr.size = (*_size);
    return entry->attr.size;
}

const struct tm *dir_entry_stamp(
        node_t *_node, const char *_stamp) {
    if (_node == NULL) {
        return NULL;
    }
    if (_node->type != NODE_TYPE_DIRENTRY) {
        return NULL;
    }
    dir_entry_t *entry = (dir_entry_t *) _node;
    if (_stamp == NULL) {
        return &entry->attr.stamp;
    }
    char *endstr = NULL;
    char tmpstr[8] = {0};
    int idx = 0;
    struct tm stamp;
    memset(&stamp, 0, sizeof(struct tm));
    // yyyy
    memcpy(tmpstr, _stamp + idx, 4);
    tmpstr[4] = 0;
    long tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return NULL;
    }
    stamp.tm_year = tmpval;
    idx += 4;
    // MM
    memcpy(tmpstr, _stamp + idx, 2);
    tmpstr[2] = 0;
    tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return NULL;
    }
    stamp.tm_mon = tmpval;
    idx += 2;
    // dd
    memcpy(tmpstr, _stamp + idx, 2);
    tmpstr[2] = 0;
    tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return NULL;
    }
    stamp.tm_mday = tmpval;
    idx += 2;
    // hh
    memcpy(tmpstr, _stamp + idx, 2);
    tmpstr[2] = 0;
    tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return NULL;
    }
    stamp.tm_hour = tmpval;
    idx += 2;
    // mm
    memcpy(tmpstr, _stamp + idx, 2);
    tmpstr[2] = 0;
    tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return NULL;
    }
    stamp.tm_min = tmpval;
    idx += 2;
    // ss
    memcpy(tmpstr, _stamp + idx, 2);
    tmpstr[2] = 0;
    tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return NULL;
    }
    stamp.tm_sec = tmpval;
    idx += 2;
    entry->attr.stamp = stamp;
    return &entry->attr.stamp;
}

typedef struct var_spec_t {
    node_t parent;
    mmsstr_t domain;
    mmsstr_t index;
} var_spec_t;

static int var_spec_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_VARSPEC) {
        return -1;
    }
    var_spec_t *variable = (var_spec_t *) _node;
    mmsstr_clear(&variable->domain);
    mmsstr_clear(&variable->index);
    free(_node);
    _node = NULL;
    return 0;
}

node_t *var_spec_create() {
    static const node_op_t nodeop = {
            var_spec_destroy,
    };
    node_t *node = (node_t *) malloc(sizeof(var_spec_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(var_spec_t));
    node->type = NODE_TYPE_VARSPEC;
    node->op = &nodeop;
    return node;
}

const char *var_spec_domain(
        node_t *_node, const char *_domain,
        unsigned int _length) {
    if (_node == NULL) {
        return NULL;
    }
    if (_node->type != NODE_TYPE_VARSPEC) {
        return NULL;
    }
    var_spec_t *variable = (var_spec_t *) _node;
    if (_domain == NULL) {
        return mmsstr_data(&variable->domain);
    }
    return mmsstr_set_data(&variable->domain, _domain, _length);
}

size_t var_spec_domain_len(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_VARSPEC) {
        return 0;
    }
    var_spec_t *variable = (var_spec_t *) _node;
    return variable->domain.length;
}

const char *var_spec_index(
        node_t *_node, const char *_index,
        unsigned int _length) {
    if (_node == NULL) {
        return NULL;
    }
    if (_node->type != NODE_TYPE_VARSPEC) {
        return NULL;
    }
    var_spec_t *variable = (var_spec_t *) _node;
    if (_index == NULL) {
        return mmsstr_data(&variable->index);
    }
    return mmsstr_set_data(&variable->index, _index, _length);
}

size_t var_spec_index_len(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_VARSPEC) {
        return 0;
    }
    var_spec_t *variable = (var_spec_t *) _node;
    return variable->index.length;
}

typedef struct acsret_t {
    node_t parent;
    xvalue_t value;
} acsret_t;

static int acsret_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_ACSRET) {
        return -1;
    }
    acsret_t *result = (acsret_t *) _node;
    xvalue_clear(&result->value);
    free(_node);
    _node = NULL;
    return 0;
}

node_t *acsret_create() {
    static const node_op_t nodeop = {
            acsret_destroy,
    };
    node_t *node = (node_t *) malloc(sizeof(acsret_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(acsret_t));
    node->type = NODE_TYPE_ACSRET;
    node->op = &nodeop;
    return NULL;
}

const xvalue_t *acsret_value(
        node_t *_node, const xvalue_t *_value) {
    if (_node == NULL) {
        return NULL;
    }
    if (_node->type != NODE_TYPE_ACSRET) {
        return NULL;
    }
    acsret_t *result = (acsret_t *) _node;
    if (_value == NULL) {
        return &result->value;
    }
    result->value = (*_value);
    return &result->value;
}


