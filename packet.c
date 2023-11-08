#include "packet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "localizer.h"
#include "node.h"

#define PKT_ERR_NULL (-1)
#define PKT_ERR_TYPE (-2)
#define PKT_ERR_FAILED (-3)

typedef struct val2str_t {
    int value;
    const char *str;
} val2str_t;

static const char *value2str(
        const val2str_t *_list, int _value) {
    if (_list == NULL) {
        return "";
    }
    const val2str_t *head = _list;
    while (head->str) {
        if (head->value == _value) {
            break;
        }
        head++;
    }
    return head->str;
}

static const char *data_errstr(int _code) {
    static val2str_t g_dataerr[] = {
            {0,  "object-invalidated"},
            {1,  "hardware-fault"},
            {2,  "temporarily-unavailable"},
            {3,  "object-access-denied"},
            {4,  "object-undefined"},
            {5,  "invalid-address"},
            {6,  "type-unsupported"},
            {7,  "type-inconsistent"},
            {8,  "object-attribute-inconsistent"},
            {9,  "object-access-unsupported"},
            {10, "object-non-existent"},
            {11, "object-value-invalid"},
            {0, NULL},
    };
    return value2str(g_dataerr, _code);
}

/*********************************file_spec_t*********************************/

typedef struct file_spec_t {
    node_t parent;
    mmsstr_t path;
} file_spec_t;

static int file_spec_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_FILESPEC) {
        return PKT_ERR_TYPE;
    }
    file_spec_t *file = (file_spec_t *) _node;
    mmsstr_clear(&file->path);
    free(_node);
    return 0;
}

static int file_spec_tostring(
        node_t *_node, char *_dest, size_t _size) {
    if (_node == NULL || _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FILESPEC) {
        return PKT_ERR_TYPE;
    }
    file_spec_t *file = (file_spec_t *) (_node);
    const char *fmt = xtrans("pathSpec:{path:%s}");
    int ret = snprintf(
            _dest, _size - 1, fmt,
            mmsstr_data(&file->path));
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    return ret;
}

static node_t *file_spec_create() {
    static const node_op_t nodeop = {
            file_spec_destroy,
            file_spec_tostring,
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

int file_spec_path(
        node_t *_node, const char *_path,
        unsigned int _length) {
    if (_node == NULL || _path == NULL || _length == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FILESPEC) {
        return PKT_ERR_TYPE;
    }
    file_spec_t *file = (file_spec_t *) _node;
    mmsstr_set_data(&file->path, _path, _length);
    return 0;
}

/*********************************dir_entry_t*********************************/

typedef struct file_attr_t {
    size_t size;
    struct tm stamp;
} file_attr_t;

static int fileattr_stamp(
        file_attr_t *_attr, const char *_stamp) {
    if (_attr == NULL || _stamp == NULL) {
        return PKT_ERR_NULL;
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
        return PKT_ERR_FAILED;
    }
    stamp.tm_year = tmpval;
    idx += 4;
    // MM
    memcpy(tmpstr, _stamp + idx, 2);
    tmpstr[2] = 0;
    tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return PKT_ERR_FAILED;
    }
    stamp.tm_mon = tmpval;
    idx += 2;
    // dd
    memcpy(tmpstr, _stamp + idx, 2);
    tmpstr[2] = 0;
    tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return PKT_ERR_FAILED;
    }
    stamp.tm_mday = tmpval;
    idx += 2;
    // hh
    memcpy(tmpstr, _stamp + idx, 2);
    tmpstr[2] = 0;
    tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return PKT_ERR_FAILED;
    }
    stamp.tm_hour = tmpval;
    idx += 2;
    // mm
    memcpy(tmpstr, _stamp + idx, 2);
    tmpstr[2] = 0;
    tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return PKT_ERR_FAILED;
    }
    stamp.tm_min = tmpval;
    idx += 2;
    // ss
    memcpy(tmpstr, _stamp + idx, 2);
    tmpstr[2] = 0;
    tmpval = strtol(tmpstr, &endstr, 10);
    if (endstr[0] != 0) {
        return PKT_ERR_FAILED;
    }
    stamp.tm_sec = tmpval;
    idx += 2;
    _attr->stamp = stamp;
    return idx;
}

static int fileattr_tostring(
        file_attr_t *_attr,
        char *_dest, size_t _size) {
    if (_attr == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    const char *fmt = xtrans("fileAttr:{size:%u, UTC_stamp:%04d-%02d-%02d %02d:%02d:%02d}");
    int ret = snprintf(
            _dest, _size - 1, fmt, _attr->size,
            _attr->stamp.tm_year, _attr->stamp.tm_mon,
            _attr->stamp.tm_mday, _attr->stamp.tm_hour,
            _attr->stamp.tm_min, _attr->stamp.tm_sec);
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    return ret;
}

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
        return PKT_ERR_TYPE;
    }
    dir_entry_t *entry = (dir_entry_t *) _node;
    mmsstr_clear(&entry->name);
    free(_node);
    _node = NULL;
    return 0;
}

static int dir_entry_tostring(
        node_t *_node, char *_dest, size_t _size) {
    if (_node == NULL || _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_DIRENTRY) {
        return PKT_ERR_TYPE;
    }
    dir_entry_t *entry = (dir_entry_t *) _node;
    const char *fmt = xtrans("directoryEntry:{path:%s, ");
    int idx = 0;
    int ret = snprintf(
            _dest, _size - 1, fmt,
            mmsstr_data(&entry->name));
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    idx += ret;
    if (ret >= (_size - 1)) {
        return idx;
    }
    ret = fileattr_tostring(
            &entry->attr, _dest + idx, _size - idx);
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    idx += ret;
    if (ret >= (_size - 1)) {
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static node_t *dir_entry_create() {
    static const node_op_t nodeop = {
            dir_entry_destroy,
            dir_entry_tostring,
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

int dir_entry_path(
        node_t *_node, const char *_path,
        unsigned int _length) {
    if (_node == NULL || _path == NULL || _length == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_DIRENTRY) {
        return PKT_ERR_TYPE;
    }
    dir_entry_t *entry = (dir_entry_t *) _node;
    mmsstr_set_data(&entry->name, _path, _length);
    return 0;
}

int dir_entry_size(
        node_t *_node, unsigned int _size) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_DIRENTRY) {
        return PKT_ERR_TYPE;
    }
    dir_entry_t *entry = (dir_entry_t *) _node;
    entry->attr.size = _size;
    return 0;
}

int dir_entry_stamp(
        node_t *_node, const char *_stamp) {
    if (_node == NULL || _stamp == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_DIRENTRY) {
        return PKT_ERR_TYPE;
    }
    dir_entry_t *entry = (dir_entry_t *) _node;
    fileattr_stamp(&entry->attr, _stamp);
    return 0;
}

/*********************************fopen_req_t*********************************/

typedef struct fopen_req_t {
    node_t parent;
    mmsstr_t path;
    unsigned int position;
} fopen_req_t;

static int fopen_req_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_FOPENREQ) {
        return PKT_ERR_TYPE;
    }
    fopen_req_t *req = (fopen_req_t *) _node;
    mmsstr_clear(&req->path);
    free(_node);
    _node = NULL;
    return 0;
}

static int fopen_req_tostring(
        node_t *_node, char *_dest, size_t _size) {
    if (_node == NULL || _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FOPENREQ) {
        return PKT_ERR_TYPE;
    }
    fopen_req_t *req = (fopen_req_t *) _node;
    const char *fmt = xtrans("fileOpenRequest:{path:%s, position:%u}");
    int ret = snprintf(
            _dest, _size - 1, fmt,
            mmsstr_data(&req->path), req->position);
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    return ret;
}

static node_t *fopen_req_create() {
    static const node_op_t nodeop = {
            fopen_req_destroy,
            fopen_req_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(fopen_req_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(fopen_req_t));
    node->type = NODE_TYPE_FOPENREQ;
    node->op = &nodeop;
    return node;
}

int fopen_req_path(
        node_t *_node, const char *_path,
        unsigned int _length) {
    if (_node == NULL || _path == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FOPENREQ) {
        return PKT_ERR_TYPE;
    }
    fopen_req_t *req = (fopen_req_t *) _node;
    mmsstr_set_data(&req->path, _path, _length);
    return 0;
}

int fopen_req_position(
        node_t *_node, unsigned int _pos) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FOPENREQ) {
        return PKT_ERR_TYPE;
    }
    fopen_req_t *req = (fopen_req_t *) _node;
    req->position = _pos;
    return 0;
}

/*********************************fopen_resp_t*********************************/

typedef struct fopen_resp_t {
    node_t parent;
    unsigned int frsm;
    file_attr_t attr;
} fopen_resp_t;

static int fopen_resp_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_FOPENRESP) {
        return PKT_ERR_TYPE;
    }
    free(_node);
    _node = NULL;
    return 0;
}

static int fopen_resp_tostring(
        node_t *_node, char *_dest, size_t _size) {
    if (_node == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FOPENRESP) {
        return PKT_ERR_TYPE;
    }
    fopen_resp_t *resp = (fopen_resp_t *) _node;
    const char *fmt = xtrans("fileOpenResponse:{fileHandle:%u, ");
    int idx = 0;
    int ret = snprintf(
            _dest + idx, _size - idx - 1,
            fmt, resp->frsm);
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    idx += ret;
    if (ret >= (_size - 1)) {
        return idx;
    }
    ret = fileattr_tostring(
            &resp->attr, _dest + idx, _size - idx);
    if (ret < 0) {
        return ret;
    }
    idx += ret;
    if (ret >= (_size - 1)) {
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static node_t *fopen_resp_create() {
    static const node_op_t nodeop = {
            fopen_resp_destroy,
            fopen_resp_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(fopen_resp_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(fopen_resp_t));
    node->type = NODE_TYPE_FOPENRESP;
    node->op = &nodeop;
    return node;
}

int fopen_resp_frsm(node_t *_node, unsigned int _frsm) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FOPENRESP) {
        return PKT_ERR_TYPE;
    }
    fopen_resp_t *resp = (fopen_resp_t *) _node;
    resp->frsm = _frsm;
    return 0;
}

int fopen_resp_attr(
        node_t *_node, unsigned int _fsize,
        const char *_stamp) {
    if (_node == NULL || _stamp == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FOPENRESP) {
        return PKT_ERR_TYPE;
    }
    fopen_resp_t *resp = (fopen_resp_t *) _node;
    resp->attr.size = _fsize;
    fileattr_stamp(&resp->attr, _stamp);
    return 0;
}

/*********************************fread_t*********************************/

typedef struct fread_t {
    node_t parent;
    unsigned int value;
} fread_t;

static int fread_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_FREAD) {
        return PKT_ERR_TYPE;
    }
    free(_node);
    _node = NULL;
    return 0;
}

static int fread_tostring(
        node_t *_node, char *_dest, size_t _size) {
    if (_node == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FREAD) {
        return PKT_ERR_TYPE;
    }
    fread_t *fread1 = (fread_t *) _node;
    const char *fmt = xtrans("fileReadRequest:{fileHandle:%u}");
    int ret = snprintf(
            _dest, _size - 1,
            fmt, fread1->value);
    return ret;
}

static node_t *fread_create() {
    static const node_op_t nodeop = {
            fread_destroy,
            fread_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(fread_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(fread_t));
    node->type = NODE_TYPE_FREAD;
    node->op = &nodeop;
    return node;
}

int fread_value(
        node_t *_node,
        unsigned int _value) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FREAD) {
        return PKT_ERR_TYPE;
    }
    fread_t *fread1 = (fread_t *) _node;
    fread1->value = _value;
    return 0;
}

/*********************************fread_resp_t*********************************/

typedef struct fread_resp_t {
    node_t parent;
    unsigned int size;
    unsigned char start[4];
    unsigned char end[4];
    char follow;
} fread_resp_t;

static int fread_resp_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_FREADRESP) {
        return PKT_ERR_TYPE;
    }
    free(_node);
    _node = NULL;
    return 0;
}

static int fread_resp_tostring(
        node_t *_node,
        char *_dest, size_t _size) {
    if (_node == NULL || _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FREADRESP) {
        return PKT_ERR_TYPE;
    }
    fread_resp_t *resp = (fread_resp_t *) _node;
    int idx = 0;
    const char *fmt = xtrans("fileReadResponse:{size:%u");
    int ret = snprintf(
            _dest + idx, _size - idx - 1, fmt,
            resp->size);
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        return idx;
    }
    do {
        if (resp->size == 0) {
            break;
        }
        fmt = xtrans(", start:0x%02x 0x%02x 0x%02x 0x%02x");
        ret = snprintf(
                _dest + idx, _size - idx - 1, fmt,
                resp->start[0], resp->start[1],
                resp->start[2], resp->start[3]);
        if (ret < 0) {
            return PKT_ERR_FAILED;
        }
        idx += ret;
        if (idx >= (_size - 1)) {
            return idx;
        }
        if (resp->size < 5) {
            break;
        }
        fmt = xtrans(", end:0x%02x 0x%02x 0x%02x 0x%02x}");
        ret = snprintf(
                _dest + idx, _size - 1, fmt,
                resp->end[0], resp->end[1],
                resp->end[2], resp->end[3]);
        if (ret < 0) {
            return PKT_ERR_FAILED;
        }
        idx += ret;
    } while (0);
    fmt = xtrans(", follow:%c}");
    ret = snprintf(
            _dest + idx, _size - idx - 1,
            fmt, resp->follow);
    idx += ret;
    return idx;
}

static node_t *fread_resp_create() {
    static const node_op_t nodeop = {
            fread_resp_destroy,
            fread_resp_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(fread_resp_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(fread_resp_t));
    node->type = NODE_TYPE_FREADRESP;
    node->op = &nodeop;
    ((fread_resp_t *) node)->follow = 'T';
    return node;
}

int fread_resp_size(node_t *_node, unsigned int _size) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FREADRESP) {
        return PKT_ERR_TYPE;
    }
    fread_resp_t *resp = (fread_resp_t *) _node;
    resp->size = _size;
    return 0;
}

int fread_resp_flag(
        node_t *_node, const unsigned char *_flag,
        unsigned char _length, unsigned char _head) {
    if (_node == NULL || _flag == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FREADRESP) {
        return PKT_ERR_TYPE;
    }
    fread_resp_t *resp = (fread_resp_t *) _node;
    unsigned char *dest = resp->end;
    if (_head) {
        dest = resp->start;
    }
    if (_length > 4) {
        _length = 4;
    }
    unsigned char idx = 0;
    while (idx < _length) {
        dest[idx] = _flag[idx];
        idx++;
    }
    return 0;
}

int fread_resp_follow(
        node_t *_node, unsigned char _follow) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FREADRESP) {
        return PKT_ERR_TYPE;
    }
    fread_resp_t *resp = (fread_resp_t *) _node;
    resp->follow = 'F';
    if (_follow) {
        resp->follow = 'T';
    }
    return 0;
}

/*********************************fclose_t*********************************/

typedef struct fclose_t {
    node_t parent;
    unsigned char b_updwon;
    unsigned int i_value;
} fclose_t;

static int fclose_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_FCLOSE) {
        return PKT_ERR_TYPE;
    }
    free(_node);
    _node = NULL;
    return 0;
}

static int fclose_tostring(
        node_t *_node, char *_dest, size_t _size) {
    if (_node == NULL || _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FCLOSE) {
        return PKT_ERR_TYPE;
    }
    fclose_t *fclose1 = (fclose_t *) _node;
    int ret = 0;
    if (fclose1->b_updwon) {
        const char *fmt = xtrans("fileCloseRequest:{fileHandle:%u}");
        ret = snprintf(_dest, _size - 1, fmt, fclose1->i_value);
    } else {
        const char *fmt = xtrans("fileCloseResponse:{success}");
        if (fclose1->i_value) {
            fmt = xtrans("fileCloseResponse:{failed}");
        }
        ret = snprintf(_dest, _size - 1, "%s", fmt);
    }
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    return ret;
}

static node_t *fclose_create() {
    static const node_op_t nodeop = {
            fclose_destroy,
            fclose_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(fclose_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(fclose_t));
    node->type = NODE_TYPE_FCLOSE;
    node->op = &nodeop;
    return node;
}

int fclose_value(
        node_t *_node, unsigned char _updown,
        unsigned int _value) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_FCLOSE) {
        return PKT_ERR_TYPE;
    }
    fclose_t *fclose1 = (fclose_t *) _node;
    fclose1->b_updwon = _updown;
    fclose1->i_value = _value;
    return 0;
}

/*********************************var_spec_t*********************************/

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
        return PKT_ERR_TYPE;
    }
    var_spec_t *variable = (var_spec_t *) _node;
    mmsstr_clear(&variable->domain);
    mmsstr_clear(&variable->index);
    free(_node);
    _node = NULL;
    return 0;
}

static int var_spec_tostring(
        node_t *_node, char *_dest, size_t _size) {
    if (_node == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_VARSPEC) {
        return PKT_ERR_TYPE;
    }
    var_spec_t *varspec = (var_spec_t *) _node;
    const char *fmt = "varSpec:{%s/%s}";
    int ret = snprintf(
            _dest, _size - 1, fmt,
            mmsstr_data(&varspec->domain),
            mmsstr_data(&varspec->index));
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    return ret;
}

static node_t *var_spec_create() {
    static const node_op_t nodeop = {
            var_spec_destroy,
            var_spec_tostring,
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

int var_spec_domain(
        node_t *_node, const char *_domain,
        unsigned int _length) {
    if (_node == NULL || _domain == NULL || _length == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_VARSPEC &&
        _node->type != NODE_TYPE_WRITREQ) {
        return PKT_ERR_TYPE;
    }
    var_spec_t *variable = (var_spec_t *) _node;
    mmsstr_set_data(&variable->domain, _domain, _length);
    return 0;
}

int var_spec_index(
        node_t *_node, const char *_index,
        unsigned int _length) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_VARSPEC &&
        _node->type != NODE_TYPE_WRITREQ) {
        return PKT_ERR_TYPE;
    }
    var_spec_t *variable = (var_spec_t *) _node;
    mmsstr_set_data(&variable->index, _index, _length);
    return 0;
}

/*********************************udata_t*********************************/

typedef struct udata_t {
    node_t parent;
    xvalue_t value;
} udata_t;

static int udata_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_UDATA) {
        return -1;
    }
    udata_t *result = (udata_t *) _node;
    xvalue_clear(&result->value);
    free(_node);
    _node = NULL;
    return 0;
}

static int udata_tostring(
        node_t *_node,
        char *_dest, size_t _size) {
    if (_node == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_UDATA) {
        return PKT_ERR_TYPE;
    }
    udata_t *data = (udata_t *) _node;
    int ret = xvalue_to_string(&data->value, _dest, _size);
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    return ret;
}

static node_t *udata_create() {
    static const node_op_t nodeop = {
            udata_destroy,
            udata_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(udata_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(udata_t));
    node->type = NODE_TYPE_UDATA;
    node->op = &nodeop;
    return node;
}

const xvalue_t *udata_value(node_t *_node, const xvalue_t *_value) {
    if (_node == NULL) {
        return NULL;
    }
    if (_node->type != NODE_TYPE_UDATA) {
        return NULL;
    }
    udata_t *result = (udata_t *) _node;
    if (_value == NULL) {
        return &result->value;
    }
    result->value = (*_value);
    return &result->value;
}

/*********************************name_req_t*********************************/

typedef struct name_req_t {
    node_t parent;
    int type;
    mmsstr_t domain;
    mmsstr_t next;
} name_req_t;

static int name_req_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_NAMEREQ) {
        return -1;
    }
    name_req_t *namereq = (name_req_t *) _node;
    mmsstr_clear(&namereq->domain);
    mmsstr_clear(&namereq->next);
    free(_node);
    _node = NULL;
    return 0;
}

static int name_req_tostring(
        node_t *_node, char *_dest, size_t _size) {
    static const val2str_t g_nr_type[] = {
            {0x00, "variable"},
            {0x02, "varList"},
            {0x08, "journal"},
            {0x09, "domain"},
            {0, NULL},
    };
    if (_node == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_NAMEREQ) {
        return PKT_ERR_TYPE;
    }
    name_req_t *nreq = (name_req_t *) _node;
    const char *type = value2str(g_nr_type, nreq->type);
    if (type == NULL) {
        return PKT_ERR_FAILED;
    }
    type = xtrans(type);
    if (nreq->type == 0x09) {
        const char *data = xtrans("vmdSpecific");
        mmsstr_set_data(&nreq->domain, data, strlen(data));
    }
    const char *fmt = xtrans("nameRequest:{type:%s, domain:%s");
    int idx = 0;
    int ret = snprintf(
            _dest + idx, _size - 1 - idx,
            fmt, type, mmsstr_data(&nreq->domain));
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    if (nreq->next.length == 0) {
        _dest[idx++] = '}';
        _dest[idx] = 0;
        return idx;
    }
    fmt = xtrans(", continueAfter:%s}");
    ret = snprintf(
            _dest + idx, _size - 1 - idx,
            fmt, mmsstr_data(&nreq->next));
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    return idx;
}

static node_t *name_req_create() {
    static const node_op_t nodeop = {
            name_req_destroy,
            name_req_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(name_req_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(name_req_t));
    node->type = NODE_TYPE_NAMEREQ;
    node->op = &nodeop;
    return node;
}

int name_req_type(node_t *_node, int _type) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_NAMEREQ) {
        return PKT_ERR_TYPE;
    }
    name_req_t *nreq = (name_req_t *) _node;
    nreq->type = _type;
    return 0;
}

int name_req_domain(
        node_t *_node,
        const char *_data,
        unsigned int _length) {
    if (_node == NULL ||
        _data == NULL || _length == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_NAMEREQ) {
        return PKT_ERR_TYPE;
    }
    name_req_t *nreq = (name_req_t *) _node;
    mmsstr_set_data(&nreq->domain, _data, _length);
    return 0;
}

int name_req_next(node_t *_node,
                  const char *_data,
                  unsigned int _length) {
    if (_node == NULL || _data == NULL || _length == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_NAMEREQ) {
        return PKT_ERR_TYPE;
    }
    name_req_t *nreq = (name_req_t *) _node;
    mmsstr_set_data(&nreq->next, _data, _length);
    return 0;
}

/*********************************idstr_t*********************************/

typedef struct idstr_t {
    node_t parent;
    mmsstr_t name;
} idstr_t;

static int idstr_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_IDSTR) {
        return PKT_ERR_TYPE;
    }
    idstr_t *idstr = (idstr_t *) _node;
    mmsstr_clear(&idstr->name);
    free(_node);
    _node = NULL;
    return 0;
}

static int idstr_tostring(
        node_t *_node,
        char *_dest, size_t _size) {
    if (_node == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_IDSTR) {
        return PKT_ERR_TYPE;
    }
    idstr_t *idstr = (idstr_t *) _node;
    const char *fmt = xtrans("id_string:{%s}");
    int ret = snprintf(
            _dest, _size - 1, fmt,
            mmsstr_data(&idstr->name));
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    return ret;
}

static node_t *idstr_create() {
    static const node_op_t nodeop = {
            idstr_destroy,
            idstr_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(idstr_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(idstr_t));
    node->type = NODE_TYPE_IDSTR;
    node->op = &nodeop;
    return node;
}

int idstr_name(
        node_t *_node,
        const char *_data,
        unsigned int _length) {
    if (_node == NULL || _data == NULL || _length == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_IDSTR) {
        return PKT_ERR_TYPE;
    }
    idstr_t *idstr = (idstr_t *) _node;
    mmsstr_set_data(&idstr->name, _data, _length);
    return 0;
}

/*********************************writ_resp_t*********************************/

typedef struct writ_resp_t {
    node_t parent;
    unsigned char is_okay;
    unsigned char code;
} writ_resp_t;

static int writ_resp_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type == NODE_TYPE_WRITRESP) {
        return PKT_ERR_TYPE;
    }
    free(_node);
    _node = NULL;
    return 0;
}

static int writ_resp_tostring(
        node_t *_node,
        char *_dest, size_t _size) {
    if (_node == NULL || _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    writ_resp_t *resp = (writ_resp_t *) _node;
    if (resp->is_okay) {
        int ret = snprintf(
                _dest, _size - 1,
                "writeResult:{success}");
        if (ret < 0) {
            return PKT_ERR_FAILED;
        }
        return ret;
    }
    const char *errstr =
            data_errstr(resp->code);
    if (errstr == NULL) {
        return 0;
    }
    errstr = xtrans(errstr);
    int ret = snprintf(
            _dest, _size - 1,
            "writeResult:{%s}", errstr);
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    return ret;
}

static node_t *writ_resp_create() {
    static const node_op_t nodeop = {
            writ_resp_destroy,
            writ_resp_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(writ_resp_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(writ_resp_t));
    node->type = NODE_TYPE_WRITRESP;
    node->op = &nodeop;
    return node;
}

int writ_resp_code(
        node_t *_node,
        unsigned char _okay,
        unsigned char _code) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_WRITRESP) {
        return PKT_ERR_TYPE;
    }
    writ_resp_t *resp = (writ_resp_t *) _node;
    if (_okay) {
        resp->is_okay = _okay;
        resp->code = 0;
        return 0;
    }
    resp->is_okay = _okay;
    resp->code = _code;
    return 0;
}

/*********************************writ_req_t*********************************/

typedef struct write_req_t {
    var_spec_t parent;
    xvalue_t value;
} writ_req_t;

static int write_req_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_WRITREQ) {
        return PKT_ERR_TYPE;
    }
    writ_req_t *req = (writ_req_t *) _node;
    mmsstr_clear(&req->parent.domain);
    mmsstr_clear(&req->parent.index);
    xvalue_clear(&req->value);
    return 0;
}

static int writ_req_tostring(
        node_t *_node,
        char *_dest, size_t _size) {
    if (_node == NULL ||
        _dest == NULL || _size == 0) {
        return 0;
    }
    if (_node->type != NODE_TYPE_WRITREQ) {
        return PKT_ERR_TYPE;
    }
    writ_req_t *req = (writ_req_t *) _node;
    int idx = 0;
    int ret = snprintf(
            _dest + idx, _size - 1 - idx,
            "writeValue:{%s/%s:",
            mmsstr_data(&req->parent.domain),
            mmsstr_data(&req->parent.index));
    if (ret < 0) {
        _dest[0] = 0;
        return PKT_ERR_FAILED;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) (_size - 1);
        return idx;
    }
    ret = xvalue_to_string(&req->value, _dest + idx, _size - idx);
    if (ret < 0) {
        _dest[idx] = 0;
        return idx;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) (_size - 1);
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static node_t *writ_req_create() {
    static const node_op_t nodeop = {
            write_req_destroy,
            writ_req_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(writ_req_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(writ_req_t));
    node->type = NODE_TYPE_WRITREQ;
    node->op = &nodeop;
    return node;
}

int writ_req_value(
        node_t *_node,
        const xvalue_t *_value) {
    if (_node == NULL || _value == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_WRITREQ) {
        return PKT_ERR_TYPE;
    }
    writ_req_t *request = (writ_req_t *) _node;
    request->value = (*_value);
    return 0;
}

/*********************************init_t*********************************/

static const char *query_service_name(int _idx) {
    static const char *g_service_name[] = {
            "status:%s,\n",
            "getNameList:%s,\n",
            "identify:%s,\n",
            "rename:%s,\n",
            "read:%s,\n",
            "write:%s,\n",
            "getVariableAccessAttributes:%s,\n",
            "defineNamedVariable:%s,\n",
            "defineScatteredAccess:%s,\n",
            "getScatteredAccessAttributes:%s,\n",
            "deleteVariableAccess:%s,\n",
            "defineNamedVariableList:%s,\n",
            "getNamedVariableListAttributes:%s,\n",
            "deleteNamedVariableList:%s,\n",
            "defineNamedType:%s,\n",
            "getNamedTypeAttributes:%s,\n",
            "deleteNamedType:%s,\n",
            "input:%s,\n",
            "output:%s,\n",
            "takeControl:%s,\n",
            "relinquishControl:%s,\n",
            "defineSemaphore:%s,\n",
            "deleteSemaphore:%s,\n",
            "reportSemaphoreStatus:%s,\n",
            "reportPoolSemaphoreStatus:%s,\n",
            "reportSemaphoreEntryStatus:%s,\n",
            "initiateDownloadSequence:%s,\n",
            "downloadSegment:%s,\n",
            "terminateDownloadSequence:%s,\n",
            "initiateUploadSequence:%s,\n",
            "uploadSegment:%s,\n",
            "terminateUploadSequence:%s,\n",
            "requestDomainDownload:%s,\n",
            "requestDomainUpload:%s,\n",
            "loadDomainContent:%s,\n",
            "storeDomainContent:%s,\n",
            "deleteDomain:%s,\n",
            "getDomainAttributes:%s,\n",
            "createProgramInvocation:%s,\n",
            "deleteProgramInvocation:%s,\n",
            "start:%s,\n",
            "stop:%s,\n",
            "resume:%s,\n",
            "reset:%s,\n",
            "kill:%s,\n",
            "getProgramInvocationAttributes:%s,\n",
            "obtainFile:%s,\n",
            "defineEventCondition:%s,\n",
            "deleteEventCondition:%s,\n",
            "getEventConditionAttributes:%s,\n",
            "reportEventConditionStatus:%s,\n",
            "alterEventConditionMonitoring:%s,\n",
            "triggerEvent:%s,\n",
            "defineEventAction:%s,\n",
            "deleteEventAction:%s,\n",
            "getEventActionAttributes:%s,\n",
            "reportActionStatus:%s,\n",
            "defineEventEnrollment:%s,\n",
            "deleteEventEnrollment:%s,\n",
            "alterEventEnrollment:%s,\n",
            "reportEventEnrollmentStatus:%s,\n",
            "getEventEnrollmentAttributes:%s,\n",
            "acknowledgeEventNotification:%s,\n",
            "getAlarmSummary:%s,\n",
            "getAlarmEnrollmentSummary:%s,\n",
            "readJournal:%s,\n",
            "writeJournal:%s,\n",
            "initializeJournal:%s,\n",
            "reportJournalStatus:%s,\n",
            "createJournal:%s,\n",
            "deleteJournal:%s,\n",
            "getCapabilityList:%s,\n",
            "fileOpen:%s,\n",
            "fileRead:%s,\n",
            "fileClose:%s,\n",
            "fileRename:%s,\n",
            "fileDelete:%s,\n",
            "fileDirectory:%s,\n",
            "unsolicitedStatus:%s,\n",
            "informationReport:%s,\n",
            "eventNotification:%s,\n",
            "attachToEventCondition:%s,\n",
            "attachToSemaphore:%s,\n",
            "conclude:%s,\n",
            "cancel:%s,\n",
    };
    if (_idx < 0 || _idx >= 85) {
        return "";
    }
    return g_service_name[_idx];
}

static int init_services_tostring(
        const unsigned char *_data,
        char *_dest, size_t _size) {
    if (_data == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    int idx = 0;
    int ret = snprintf(
            _dest + idx, _size - idx - 1,
            "\nservicesSupportedCalled:{\n");
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    idx += ret;
    ret = 0;
    while (ret < 85 && idx < (_size - 1)) {
        int byte_idx = (ret >> 3);
        int bit_idx = (ret & 0x07);
        unsigned char flag = _data[byte_idx];
        flag &= (0x80 >> bit_idx);
        const char *b_val = "false";
        if (flag) {
            b_val = "true";
        }
        const char *name = query_service_name(ret);
        int len = snprintf(
                _dest + idx, _size - idx - 1,
                name, b_val);
        if (len < 0) {
            return PKT_ERR_FAILED;
        }
        idx += len;
        ret++;
    }
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static const char *query_cbb_name(int _idx) {
    static const char *cbb_name[] = {
            "str1:%s,\n", "str2:%s,\n", "vnam:%s,\n", "valt:%s,\n",
            "vadr:%s,\n", "vsca:%s,\n", "tpy:%s,\n", "vlis:%s,\n",
            "real:%s,\n", "spare_bit9:%s,\n", "cei:%s,\n",
    };
    if (_idx < 0 || _idx >= 11) {
        return "";
    }
    return cbb_name[_idx];
}

static int init_param_cbb_tostring(
        const unsigned char *_data,
        char *_dest, size_t _size) {
    if (_data == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    int idx = 0;
    int ret = snprintf(
            _dest + idx, _size - idx - 1,
            "paramterCBB:{\n");
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    idx += ret;
    ret = 0;
    while (ret < 11 && idx < (_size - 1)) {
        int byte_idx = (ret >> 3);
        int bit_idx = (ret & 0x07);
        unsigned char flag = _data[byte_idx];
        flag &= (0x80 >> bit_idx);
        const char *b_val = "false";
        if (flag) {
            b_val = "true";
        }
        const char *name = query_cbb_name(ret);
        int len = snprintf(
                _dest + idx, _size - idx - 1,
                name, b_val);
        if (len < 0) {
            return PKT_ERR_FAILED;
        }
        idx += len;
        ret++;
    }
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

typedef struct init_t {
    node_t parent;
    // local detail calling
    unsigned int detail;
    // max serv outstanding calling
    unsigned char max_calling;
    // max serv outstanding called
    unsigned char max_called;
    // data structure nesting level
    unsigned char nest_level;
    // version number
    unsigned char version;
    // reuqest or response
    unsigned char updown;
    // paramerter CBB
    unsigned char padding1;
    unsigned char param_cbb[2];
    // service supported calling
    unsigned char padding2;
    unsigned char callings[11];
} init_t;

static int init_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_INIT) {
        return PKT_ERR_TYPE;
    }
    free(_node);
    _node = NULL;
    return 0;
}

static int init_detail_tostring(
        init_t *_init,
        char *_dest, size_t _size) {
    if (_init == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    int idx = 0;
    // version number
    int ret = snprintf(
            _dest + idx, _size - idx - 1,
            "InitializeDetail:{\nversion: %u,\n",
            _init->version);
    if (ret < 0) {
        return PKT_ERR_TYPE;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    // parameter CBB
    ret = init_param_cbb_tostring(
            _init->param_cbb,
            _dest + idx, _size - idx);
    if (ret < 0) {
        return ret;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    // service supported called
    ret = init_services_tostring(
            _init->callings,
            _dest + idx, _size - idx);
    if (ret < 0) {
        return ret;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    _dest[idx++] = '\n';
    _dest[idx] = 0;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static int init_tostring(
        node_t *_node,
        char *_dest, size_t _size) {
    if (_node == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_INIT) {
        return PKT_ERR_TYPE;
    }
    init_t *init = (init_t *) _node;
    int idx = 0;
    int ret = snprintf(
            _dest + idx, _size - idx - 1,
            "InitializePDU:{\n"
            "localDetailCalling:%u,\n",
            init->detail);
    if (ret < 0) {
        return PKT_ERR_TYPE;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    ret = snprintf(
            _dest + idx, _size - idx - 1,
            "maxCalling:%u,\n", init->max_calling);
    if (ret < 0) {
        return PKT_ERR_TYPE;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    ret = snprintf(
            _dest + idx, _size - idx - 1,
            "maxCalled:%u,\n", init->max_called);
    if (ret < 0) {
        return PKT_ERR_TYPE;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    ret = snprintf(
            _dest + idx, _size - idx - 1,
            "structNestLevel:%u,\n", init->nest_level);
    if (ret < 0) {
        return PKT_ERR_TYPE;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    ret = init_detail_tostring(
            init, _dest + idx, _size - idx);
    if (ret < 0) {
        return ret;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    _dest[idx++] = '\n';
    _dest[idx] = 0;
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static node_t *init_create() {
    static const node_op_t nodeop = {
            init_destroy,
            init_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(init_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(init_t));
    node->type = NODE_TYPE_INIT;
    node->op = &nodeop;
    return node;
}

int init_detail_called(
        node_t *_node,
        unsigned int _value) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_INIT) {
        return PKT_ERR_TYPE;
    }
    init_t *init = (init_t *) _node;
    init->detail = _value;
    return 0;
}

int init_max_calling(
        node_t *_node,
        unsigned int _calling) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_INIT) {
        return PKT_ERR_TYPE;
    }
    init_t *init = (init_t *) _node;
    init->max_calling = _calling;
    return 0;
}

int init_max_called(
        node_t *_node,
        unsigned int _called) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_INIT) {
        return PKT_ERR_TYPE;
    }
    init_t *init = (init_t *) _node;
    init->max_called = _called;
    return 0;
}

int init_struct_nest(
        node_t *_node,
        unsigned int _nest) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_INIT) {
        return PKT_ERR_TYPE;
    }
    init_t *init = (init_t *) _node;
    init->nest_level = _nest;
    return 0;
}

int init_version(
        node_t *_node,
        unsigned int _version) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_INIT) {
        return PKT_ERR_TYPE;
    }
    init_t *init = (init_t *) _node;
    init->version = _version;
    return 0;
}

int init_param_cbb(
        node_t *_node,
        const unsigned char *_data) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_INIT) {
        return PKT_ERR_TYPE;
    }
    init_t *init = (init_t *) _node;
    int idx = 0;
    init->padding1 = _data[idx++];
    init->param_cbb[0] = _data[idx++];
    init->param_cbb[1] = _data[idx++];
    return 0;
}

int init_services(
        node_t *_node,
        const unsigned char *_data) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_INIT) {
        return PKT_ERR_TYPE;
    }
    init_t *init = (init_t *) _node;
    init->padding2 = _data[0];
    memcpy(init->callings, _data + 1, 11);
    return 0;
}

/*********************************type_spec_t*********************************/

typedef struct type_spec_t {
    node_t parent;
    mmsstr_t name;
    int code;
    xvalue_t type; // constraint
} type_spec_t;

static int type_destroy(node_t *_node) {
    if (_node == NULL) {
        return 0;
    }
    if (_node->type != NODE_TYPE_TYPE) {
        return PKT_ERR_TYPE;
    }
    type_spec_t *type = (type_spec_t *) _node;
    mmsstr_clear(&type->name);
    xvalue_clear(&type->type);
    free(_node);
    _node = NULL;
    return 0;
}

static int type_tostring(
        node_t *_node,
        char *_dest, size_t _size) {
    static const val2str_t val2type[] = {
            {0x85, "integer"},
            {0x86, "unsigned integer"},
            {0x84, "bit-string"},
            {0x90, "unicode"},
            {0x8a, "string"},
            {0x83, "boolean"},
            {0x91, "UTC-time"},
            {0xa7, "float"},
            {0, NULL},
    };
    if (_node == NULL ||
        _dest == NULL || _size == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_TYPE) {
        return PKT_ERR_TYPE;
    }
    type_spec_t *type = (type_spec_t *) _node;
    int idx = 0;
    int ret = snprintf(
            _dest + idx, _size - 1 - idx,
            "Attribute:{name:%s",
            mmsstr_data(&type->name));
    if (ret < 0) {
        return PKT_ERR_FAILED;
    }
    idx += ret;
    if (type->code == 0xa2) {
        xlist_t *list = type->type.value._struct;
        node_t *node = xlist_begin(list);
        while (node && idx < (_size - 1)) {
            _dest[idx++] = ',';
            ret = node_tostring(
                    node, _dest + idx, _size - idx);
            if (ret <= 0) {
                break;
            }
            idx += ret;
            node = xlist_next(list);
        }
    } else {
        const char *type_str = value2str(
                val2type, type->code);
        if (type_str == NULL) {
            return PKT_ERR_FAILED;
        }
        ret = snprintf(
                _dest + idx, _size - 1 - idx,
                ", type:%s", type_str);
        if (ret < 0) {
            return PKT_ERR_FAILED;
        }
        idx += ret;
        if (idx >= (_size - 1)) {
            idx = (int) _size - 1;
            return idx;
        }
        if (type->code == 0x85 || type->code == 0x86 ||
            type->code == 0x84 || type->code == 0x90 ||
            type->code == 0x8a) {
            unsigned int max_len =
                    type->type.value._uint;
            ret = snprintf(
                    _dest + idx, _size - idx - 1,
                    ", length:%u", max_len);
            if (ret < 0) {
                return PKT_ERR_FAILED;
            }
            idx += ret;
        }
    }
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static node_t *type_create() {
    static const node_op_t nodeop = {
            type_destroy,
            type_tostring,
    };
    node_t *node = (node_t *) malloc(sizeof(type_spec_t));
    if (node == NULL) {
        return node;
    }
    memset(node, 0, sizeof(type_spec_t));
    node->type = NODE_TYPE_TYPE;
    node->op = &nodeop;
    return node;
}

int type_name(
        node_t *_node, const char *_name,
        unsigned int _length) {
    if (_node == NULL ||
        _name == NULL || _length == 0) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_TYPE) {
        return PKT_ERR_TYPE;
    }
    type_spec_t *type = (type_spec_t *) _node;
    mmsstr_set_data(&type->name, _name, _length);
    return 0;
}

int type_code(node_t *_node, int _code) {
    if (_node == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_TYPE) {
        return PKT_ERR_TYPE;
    }
    type_spec_t *type = (type_spec_t *) _node;
    type->code = _code;
    return 0;
}

int type_constraint(
        node_t *_node,
        const xvalue_t *_value) {
    if (_node == NULL || _value == NULL) {
        return PKT_ERR_NULL;
    }
    if (_node->type != NODE_TYPE_TYPE) {
        return PKT_ERR_TYPE;
    }
    type_spec_t *type = (type_spec_t *) _node;
    type->type = (*_value);
    return 0;
}

xlist_t *type_get_constraint(
        node_t *_node) {
    if (_node == NULL ||
        _node->type != NODE_TYPE_TYPE) {
        return NULL;
    }
    type_spec_t *type = (type_spec_t *) _node;
    return type->type.value._struct;
}

/*********************************node*********************************/

typedef struct node_creat_t {
    int type;

    node_t *(*create)();
} node_creat_t;

node_t *node_create(int _type) {
    static const node_creat_t g_node_creat[] = {
            {NODE_TYPE_FILESPEC,  file_spec_create},
            {NODE_TYPE_DIRENTRY,  dir_entry_create},
            {NODE_TYPE_VARSPEC,   var_spec_create},
            {NODE_TYPE_UDATA,     udata_create},
            {NODE_TYPE_NAMEREQ,   name_req_create},
            {NODE_TYPE_IDSTR,     idstr_create},
            {NODE_TYPE_WRITRESP,  writ_resp_create},
            {NODE_TYPE_WRITREQ,   writ_req_create},
            {NODE_TYPE_FOPENREQ,  fopen_req_create},
            {NODE_TYPE_FOPENRESP, fopen_resp_create},
            {NODE_TYPE_FREAD,     fread_create},
            {NODE_TYPE_FCLOSE,    fclose_create},
            {NODE_TYPE_FREADRESP, fread_resp_create},
            {NODE_TYPE_INIT,      init_create},
            {NODE_TYPE_TYPE,      type_create},
            {0, NULL},
    };
    const node_creat_t *constructor = g_node_creat;
    while (constructor->create) {
        if (constructor->type == _type) {
            break;
        }
        constructor++;
    }
    if (constructor->create == NULL) {
        return NULL;
    }
    return constructor->create();
}
