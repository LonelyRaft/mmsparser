
#include "parser.h"
#include "localizer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MMS_ERR_NULL (-1)
#define MMS_ERR_FLAG (-2)
#define MMS_ERR_LENGTH (-3)
#define MMS_ERR_DATATYPE (-4)
#define MMS_ERR_MSGTYPE (-5)
#define MMS_ERR_INVOKE (-6)
#define MMS_ERR_REQTYPE (-7)
#define MMS_ERR_RESPTYPE (-8)
#define MMS_ERR_MEMALLOC (-9)
#define MMS_ERR_DATANODE (-10)
#define MMS_ERR_DOMAIN (-11)
#define MMS_ERR_DEPTH (-12)

// #define MMS_MSG_INVALID (0x00)
#define MMS_MSG_REQUEST (0xa0)
#define MMS_MSG_RESPONSE (0xa1)
#define MMS_MSG_REPORT (0xa3)
#define MMS_MSG_INIT_REQ (0xa8)
#define MMS_MSG_INIT_RESP (0xa9)

typedef struct service_op_t {
    int (*destroy)(service_t *);

    int (*tostring)(const service_t *, char *, size_t);
} service_op_t;

typedef struct service_t {
    int type;
    int code;
    unsigned int index;
    const service_op_t *op;
} service_t;

typedef struct initdata_t {
    service_t parent;
    node_t *data;
} initdata_t;

typedef struct request_t {
    service_t parent;
    unsigned int invoke;
    int type; // request type
    union reqdata {
        xlist_t *list;
        node_t *node;
    } data;
} request_t;

typedef struct response_t {
    service_t parent;
    unsigned int invoke;
    int type; // response type
    union respdata {
        xlist_t *list;
        node_t *node;
    } data;
    unsigned char follow_has; // has follow flag
    unsigned char follow_is; // value of the follow flag
    unsigned char delete; // deletable flag
} response_t;

typedef struct report_t {
    service_t parent;
    xlist_t *data;
} report_t;

int mms_tostring(
        const service_t *_serice,
        char *_dest, size_t _size) {
    if (_serice == NULL ||
        _dest == NULL || _size == 0) {
        return MMS_ERR_NULL;
    }
    int ret = 0;
    if (_serice->code != 0) {
        const char *fmt = xtrans(
                "message parsing error:"
                "{error:%s, position:%u}");
        ret = snprintf(
                _dest, _size - 1, fmt,
                error_tostring(_serice->code),
                _serice->index);
        if (ret < 0) {
            return ret;
        }
        return ret;
    }
    if (_serice->op == NULL ||
        _serice->op->tostring == NULL) {
        return ret;
    }
    return _serice->op->tostring(_serice, _dest, _size);
}

int mms_destroy(service_t *_service) {
    if (_service == NULL) {
        return 0;
    }
    if (_service->op == NULL ||
        _service->op->destroy == NULL) {
        return 0;
    }
    return _service->op->destroy(_service);
}

typedef struct errstr_t {
    int err_code;
    const char *err_str;
} errstr_t;

const char *error_tostring(int _error) {
    static const errstr_t g_errstr[] = {
            {MMS_ERR_NULL,     "MMS_ERR_NULL"},
            {MMS_ERR_FLAG,     "MMS_ERR_FLAG"},
            {MMS_ERR_LENGTH,   "MMS_ERR_LENGTH"},
            {MMS_ERR_DATATYPE, "MMS_ERR_DATATYPE"},
            {MMS_ERR_MSGTYPE,  "MMS_ERR_MSGTYPE"},
            {MMS_ERR_INVOKE,   "MMS_ERR_INVOKE"},
            {MMS_ERR_REQTYPE,  "MMS_ERR_REQTYPE"},
            {MMS_ERR_MEMALLOC, "MMS_ERR_MEMALLOC"},
            {MMS_ERR_DATANODE, "MMS_ERR_DATANODE"},
            {0,                0},
    };
    const errstr_t *e = g_errstr;
    while (e->err_str) {
        if (e->err_code == _error) {
            break;
        }
        e++;
    }
    return e->err_str;
}

#define MMS_INVOKE_ID (0x02)

#define MMS_SERVICE_FOPEN (0x48)
#define MMS_SERVICE_FREAD (0x49)
#define MMS_SERVICE_FCLOSE (0x4a)
#define MMS_SERVICE_FILEDIR (0x4d)
#define MMS_SERVICE_NAMES (0xa1)
#define MMS_SERVICE_READ (0xa4)
#define MMS_SERVICE_WRITE (0xa5)
#define MMS_SERVICE_VARATTR (0xa6)
#define MMS_SERVICE_VARIDX (0xac)

// 解析报文长度
static int mms_parse_length(
        const unsigned char *_data,
        unsigned int *_length) {
    if (_data == NULL) {
        return -1;
    }
    int size = 0;
    unsigned int length = 0;
    if (_data[0] < 0x81) {
        length = _data[0];
        size = 1;
    } else if (_data[0] == 0x81) {
        length = _data[1];
        size = 2;
    } else if (_data[0] == 0x82) {
        length = _data[1];
        length <<= 8;
        length += _data[2];
        size = 3;
    } else {
        // log: unknown length type
        return -2;
    }
    if (_length != NULL) {
        (*_length) = length;
    }
    return size;
}

// 解析调用 ID
static int mms_parse_invoke(
        const unsigned char *_data,
        unsigned int *_invoke) {
    if (_data == NULL) {
        return -1;
    }
    int idx = 0;
    if (_data[idx++] != MMS_INVOKE_ID) {
        return -2;
    }
    unsigned int inv_size = _data[idx++];
    if (inv_size > sizeof(unsigned int)) {
        // log: invoke out of range
        return -3;
    }
    unsigned int invoke = 0;
    unsigned int inv_idx = 0;
    while (inv_idx < inv_size) {
        invoke <<= 8;
        invoke += _data[idx++];
        inv_idx++;
    }
    if (_invoke != NULL) {
        (*_invoke) = invoke;
    }
    return idx;
}

static int mms_parse_domain(
        const unsigned char *_data,
        node_t *_domain) {
    if (_data == NULL || _domain == NULL) {
        return MMS_ERR_NULL;
    }
    int idx = 0;
    // domain flag
    if (_data[idx++] != 0xa1) {
        return MMS_ERR_FLAG;
    }
    unsigned int total_len = 0;
    int ret = mms_parse_length(_data + idx, &total_len);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    total_len += idx;
    // domain id flag
    if (_data[idx++] != 0x1a) {
        return MMS_ERR_FLAG;
    }
    unsigned int length = 0;
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (_data[idx + length] != 0x1a) {
        return MMS_ERR_FLAG;
    }
    var_spec_domain(_domain, (char *) _data + idx, length);
    idx += (int) length;
    // item id flag
    if (_data[idx++] != 0x1a) {
        return MMS_ERR_FLAG;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (length != (total_len - idx)) {
        return MMS_ERR_LENGTH;
    }
    var_spec_index(_domain, (char *) _data + idx, length);
    idx += (int) length;
    return idx;
}

// 解析指定的变量
static int mms_var_spec(
        const unsigned char *_data,
        node_t *_variable) {
    if (_data == NULL || _variable == NULL) {
        return MMS_ERR_NULL;
    }
    int idx = 0;
    if (_data[idx++] != 0x30) {
        return MMS_ERR_FLAG;
    }
    unsigned int total_len = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    total_len = length + idx;
    // name flag
    if (_data[idx++] != 0xa0) {
        return MMS_ERR_FLAG;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (length != (total_len - idx)) {
        return MMS_ERR_LENGTH;
    }
    ret = mms_parse_domain(_data + idx, _variable);
    if (ret < 0) {
        return MMS_ERR_DOMAIN;
    }
    idx += ret;
    return idx;
}

// 解析读变量请求
static void mms_read_request(
        request_t *_request,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _request;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    // read request service data: 0xa1
    if (_data[idx++] != 0xa1) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    // data list flag: 0xa0
    if (_data[idx++] != 0xa0) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    xlist_t *list = xlist_create();
    if (list == NULL) {
        service->code = MMS_ERR_MEMALLOC;
        service->index += idx;
        return;
    }
    while (idx < length) {
        node_t *variable = node_create(NODE_TYPE_VARSPEC);
        if (variable == NULL) {
            break;
        }
        ret = mms_var_spec(_data + idx, variable);
        if (ret <= 0) {
            node_destroy(variable);
            variable = NULL;
            break;
        }
        xlist_append(list, variable);
        idx += ret;
    }
    service->index += idx;
    if (xlist_count(list) == 0) {
        xlist_destroy(list);
        list = NULL;
    }
    _request->data.list = list;
}

static int mms_data_value(
        const unsigned char *_data,
        xvalue_t *_value, int _level) {
    if (_level > 15) {
        return MMS_ERR_DEPTH;
    }
    if (_data == NULL || _value == NULL) {
        return MMS_ERR_NULL;
    }
    _level++;
    int idx = 0;
    switch (_data[idx++]) {
        case 0x83: {  // boolean
            if (_data[idx++] != 0x01) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            xvalue_set_bool(_value, _data[idx++]);
            break;
        }
        case 0x84: {  // bit string
            unsigned int length = 0;
            int ret = mms_parse_length(_data + idx, &length);
            if (ret <= 0) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            idx += ret;
            // bit string padding
            if (_data[idx] >= 8) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            xvalue_set_bitstr(_value, (char *) _data + idx, length);
            idx += (int) length;
            break;
        }
        case 0x85: {  // integer
            unsigned int intsz = _data[idx++];
            if (intsz > sizeof(int)) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            int i_val = 0;
            unsigned int byte_idx = 0;
            while (byte_idx < intsz) {
                i_val <<= 8;
                i_val += _data[idx++];
                byte_idx++;
            }
            xvalue_set_int(_value, i_val);
            break;
        }
        case 0x86: {  // unsigned integer
            unsigned int intsz = _data[idx++];
            if (intsz > sizeof(unsigned int)) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            unsigned int ui_val = 0;
            unsigned int byte_idx = 0;
            while (byte_idx < intsz) {
                ui_val <<= 8;
                ui_val += _data[idx++];
                byte_idx++;
            }
            xvalue_set_uint(_value, ui_val);
            break;
        }
        case 0x87: {  // float point
            if (_data[idx++] != 0x05) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            if (_data[idx++] != 0x08) {
                idx = MMS_ERR_FLAG;
                break;
            }
            float f_val;
            unsigned char tmpdata[4] = {0};
            tmpdata[3] = _data[idx++];
            tmpdata[2] = _data[idx++];
            tmpdata[1] = _data[idx++];
            tmpdata[0] = _data[idx++];
            memcpy(&f_val, tmpdata, 4);
            xvalue_set_float(_value, f_val);
            break;
        }
        case 0x89: {  // octet string
            unsigned int length = 0;
            int ret = mms_parse_length(_data + idx, &length);
            if (ret <= 0) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            idx += ret;
            xvalue_set_octstr(_value, (char *) _data + idx, length);
            idx += (int) length;
            break;
        }
        case 0x8a: {  // visible string
            unsigned int length = 0;
            int ret = mms_parse_length(_data + idx, &length);
            if (ret <= 0) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            idx += ret;
            xvalue_set_string(_value, (char *) _data + idx, length);
            idx += (int) length;
            break;
        }
        case 0x8c: {  // binary time
            unsigned int btsz = _data[idx++];
            if (btsz != 0x06) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            xvalue_set_bintime(_value, _data + idx);
            idx += 0x06;
            break;
        }
        case 0x91: {  // utc time
            if (_data[idx++] != 0x08) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            xvalue_set_utctime(_value, _data + idx);
            idx += 0x08;
            break;
        }
        case 0xa2: {
            unsigned int length = 0;
            int ret = mms_parse_length(_data + idx, &length);
            if (ret <= 0) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            idx += ret;
            length += idx;
            xlist_t *nodelist = xlist_create();
            if (nodelist == NULL) {
                idx = MMS_ERR_MEMALLOC;
                break;
            }
            while (idx < length) {
                node_t *variable = node_create(NODE_TYPE_UDATA);
                if (variable == NULL) {
                    break;
                }
                xvalue_t value;
                memset(&value, 0, sizeof(xvalue_t));
                ret = mms_data_value(_data + idx, &value, _level);
                if (ret <= 0) {
                    node_destroy(variable);
                    variable = NULL;
                    break;
                }
                udata_value(variable, &value);
                xlist_append(nodelist, variable);
                idx += ret;
            }
            xvalue_set_struct(_value, nodelist);
            break;
        }
        default: {
            idx = MMS_ERR_DATATYPE;
            break;
        }
    }
    return idx;
}

static int mms_access_result(
        const unsigned char *_data,
        node_t *_acsret) {
    if (_data == NULL || _acsret == NULL) {
        return MMS_ERR_NULL;
    }
    int idx = 0;
    xvalue_t value;
    memset(&value, 0, sizeof(xvalue_t));
    if (_data[idx] == 0x80) {  // error code
        idx++;
        if (_data[idx++] != 0x01) {
            return MMS_ERR_LENGTH;
        }
        value.value._int = _data[idx++];
        udata_value(_acsret, &value);
        return idx;
    }
    // data value
    idx = mms_data_value(_data + idx, &value, 1);
    if (idx <= 0) {
        return idx;
    }
    udata_value(_acsret, &value);
    return idx;
}

// 解析读服务响应
static void mms_read_response(
        response_t *_resp,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _resp;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    // read service response data flag
    if (_data[idx++] != 0xa1) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    xlist_t *list = xlist_create();
    if (list == NULL) {
        service->code = MMS_ERR_MEMALLOC;
        service->index += idx;
        return;
    }
    while (idx < _length) {
        node_t *result = node_create(NODE_TYPE_UDATA);
        if (result == NULL) {
            break;
        }
        ret = mms_access_result(_data + idx, result);
        if (ret <= 0) {
            node_destroy(result);
            result = NULL;
        }
        xlist_append(list, result);
        idx += ret;
    }
    service->index += idx;
    if (xlist_count(list) == 0) {
        xlist_destroy(list);
        list = NULL;
    }
    _resp->data.list = list;
}

static void mms_write_request(
        request_t *_request,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _request;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    // list flag
    if (_data[idx++] != 0xa0) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    // data list flag
    if (_data[length + idx] != 0xa0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    if (_request->data.list == NULL) {
        _request->data.list = xlist_create();
    }
    if (_request->data.list == NULL) {
        service->code = MMS_ERR_MEMALLOC;
        service->index += idx;
        return;
    }
    int idx_end = idx + (int) length;
    while (idx < idx_end) {
        node_t *req = node_create(NODE_TYPE_WRITREQ);
        ret = mms_var_spec(_data + idx, req);
        if (ret <= 0) {
            node_destroy(req);
            req = NULL;
            break;
        }
        xlist_append(_request->data.list, req);
        idx += ret;
    }
    // write request data list
    if (_data[idx++] != 0xa0) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    node_t *req = xlist_begin(_request->data.list);
    do {
        xvalue_t value;
        memset(&value, 0, sizeof(xvalue_t));
        ret = mms_data_value(_data + idx, &value, 1);
        if (ret <= 0) {
            break;
        }
        writ_req_value(req, &value);
        req = xlist_next(_request->data.list);
        idx += ret;
    } while (idx < _length);
    service->index += idx;
}

static void mms_write_response(
        response_t *_resp,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _resp;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    if (_resp->data.list == NULL) {
        _resp->data.list = xlist_create();
    }
    if (_resp->data.list == NULL) {
        service->code = MMS_ERR_MEMALLOC;
        service->index += idx;
        return;
    }
    while (idx < _length) {
        node_t *resp = node_create(NODE_TYPE_WRITRESP);
        if (resp == NULL) {
            break;
        }
        unsigned char is_okay = _data[idx++];
        if (is_okay == 0x81) {
            if (_data[idx++] != 0x00) {
                break;
            }
            writ_resp_code(resp, 1, 0);
            xlist_append(_resp->data.list, resp);
        } else if (is_okay == 0x80) {
            if (_data[idx++] != 0x01) {
                break;
            }
            unsigned char code = _data[idx++];
            writ_resp_code(resp, 0, code);
            xlist_append(_resp->data.list, resp);
        } else {
            break;
        }
    }
    service->index += idx;
}

static int mms_name_req(
        const unsigned char *_data,
        unsigned int _length,
        node_t *_name_req) {
    if (_data == NULL || _name_req == NULL) {
        return MMS_ERR_NULL;
    }
    int idx = 0;
    if (_data[idx++] != 0xa1) {
        return MMS_ERR_FLAG;
    }
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (length != (_length - idx) &&
        _data[idx + length] != 0x82) {
        return MMS_ERR_LENGTH;
    }
    if (_data[idx++] != 0x81) {
        return MMS_ERR_FLAG;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (length != (_length - idx) &&
        _data[idx + length] != 0x82) {
        return MMS_ERR_LENGTH;
    }
    name_req_domain(_name_req, (char *) _data + idx, length);
    idx += (int) length;
    if (idx == _length) {
        return idx;
    }
    if (_data[idx++] != 0x82) {
        return MMS_ERR_FLAG;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return MMS_ERR_LENGTH;
    }
    name_req_next(_name_req, (char *) _data + idx, length);
    idx += (int) length;
    return idx;
}

static void mms_getnamelist_request(
        request_t *_request,
        const unsigned char *_data,
        size_t _length) {
    int idx = 0;
    service_t *service = (service_t *) _request;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    if (_data[idx++] != 0xa0) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    if (_data[idx++] != 0x03) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    if (_data[idx++] != 0x80) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    if (_data[idx++] != 0x01) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    int type = _data[idx++];
    if (type == 0x09) {
        if (_data[idx++] != 0xa1) {
            service->code = MMS_ERR_FLAG;
            service->index += idx;
            return;
        }
        if (_data[idx++] != 0x02) {
            service->code = MMS_ERR_LENGTH;
            service->index += idx;
            return;
        }
        if (_data[idx++] != 0x80) {
            service->code = MMS_ERR_FLAG;
            service->index += idx;
            return;
        }
        if (_data[idx++] != 0x00) {
            service->code = MMS_ERR_LENGTH;
            service->index += idx;
            return;
        }
        node_t *req = node_create(NODE_TYPE_NAMEREQ);
        if (req == NULL) {
            service->code = MMS_ERR_MEMALLOC;
            service->index += idx;
            return;
        }
        name_req_type(req, type);
        _request->data.node = req;
        service->index += idx;
    } else if (type == 0x00 || type == 0x02 ||
               type == 0x08) {
        node_t *req = node_create(NODE_TYPE_NAMEREQ);
        if (req == NULL) {
            service->code = MMS_ERR_MEMALLOC;
            service->index += idx;
            return;
        }
        name_req_type(req, type);
        ret = mms_name_req(_data + idx, _length - idx, req);
        if (ret < 0) {
            node_destroy(req);
            req = NULL;
            service->code = ret;
            service->index += idx;
            return;
        }
        _request->data.node = req;
        idx += ret;
        service->index += idx;
    } else {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
}

static int mms_identifer(
        const unsigned char *_data,
        node_t *_idstr) {
    if (_data == NULL || _idstr == NULL) {
        return MMS_ERR_NULL;
    }
    int idx = 0;
    if (_data[idx++] != 0x1a) {
        return MMS_ERR_FLAG;
    }
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    idstr_name(_idstr, (char *) _data + idx, length);
    idx += (int) length;
    return idx;
}

static void mms_getnamelist_response(
        response_t *_resp,
        const unsigned char *_data,
        size_t _length) {
    int code = MMS_ERR_LENGTH;
    int idx = 0;
    service_t *service = (service_t *) _resp;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    // name list flag
    if (_data[idx++] != 0xa0) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx) &&
        _data[length + idx] != 0x81) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    if (_resp->data.list == NULL) {
        _resp->data.list = xlist_create();
    }
    if (_resp->data.list == NULL) {
        service->code = MMS_ERR_MEMALLOC;
        service->index += idx;
        return;
    }
    while (idx < _length) {
        node_t *idstr = node_create(NODE_TYPE_IDSTR);
        ret = mms_identifer(_data + idx, idstr);
        if (ret <= 0) {
            node_destroy(idstr);
            break;
        }
        idx += ret;
        xlist_append(_resp->data.list, idstr);
    }
    if (idx >= _length) {
        service->index += idx;
        return;
    }
    if (_data[idx++] != 0x81) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    if (_data[idx++] != 0x01) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    _resp->follow_has = 1;
    _resp->follow_is = _data[idx++];
    service->index += idx;
}

static void mms_varattr_request(
        request_t *_request,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _request;
    node_t *varspec = NULL;
    int code = MMS_ERR_LENGTH;
    int idx = 0;
    unsigned int length = 0;
    do {
        int ret = mms_parse_length(_data + idx, &length);
        if (ret <= 0) {
            break;
        }
        idx += ret;
        if (length != (_length - idx)) {
            break;
        }
        if (_data[idx++] != 0xa0) {
            code = MMS_ERR_FLAG;
            break;
        }
        ret = mms_parse_length(_data + idx, &length);
        if (ret <= 0) {
            break;
        }
        idx += ret;
        if (length != (_length - idx)) {
            break;
        }
        varspec = node_create(NODE_TYPE_VARSPEC);
        if (varspec == NULL) {
            code = MMS_ERR_MEMALLOC;
            break;
        }
        ret = mms_parse_domain(_data + idx, varspec);
        if (ret < 0) {
            code = MMS_ERR_DOMAIN;
            break;
        }
        idx += ret;
        _request->data.node = varspec;
        service->index += idx;
        return;
    } while (0);
    service->code = code;
    service->index += idx;
    node_destroy(varspec);
    varspec = NULL;
}

static int mms_type_array(
        node_t *_type,
        const unsigned char *_data,
        unsigned int _length, int _level);

static int mms_type_spec(
        node_t *_type,
        const unsigned char *_data,
        int _level) {
    if (_level > 9) {
        return MMS_ERR_DEPTH;
    }
    if (_type == NULL || _data == NULL) {
        return MMS_ERR_NULL;
    }
    int idx = 0;
    // item flag
    if (_data[idx++] != 0x30) {
        return MMS_ERR_FLAG;
    }
    unsigned int total = 0;
    int ret = mms_parse_length(_data + idx, &total);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    total += idx;
    // name flag
    if (_data[idx++] != 0x80) {
        return MMS_ERR_FLAG;
    }
    unsigned int length = 0;
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (_data[idx + length] != 0xa1) {
        return MMS_ERR_LENGTH;
    }
    type_name(_type, (char *) _data + idx, length);
    idx += (int) length;
    // value
    if (_data[idx++] != 0xa1) {
        return MMS_ERR_FLAG;
    }
    ret = mms_parse_length(
            _data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (length != (total - idx)) {
        return MMS_ERR_LENGTH;
    }
    int typecode = _data[idx++];
    type_code(_type, typecode);
    if (typecode == 0xa2) {
        // complex structure
        ret = mms_parse_length(
                _data + idx, &length);
        if (ret <= 0) {
            return MMS_ERR_LENGTH;
        }
        idx += ret;
        ret = mms_type_array(
                _type, _data + idx,
                length, _level + 1);
        if (ret <= 0) {
            return ret;
        }
        idx += ret;
    } else if (typecode == 0x85 ||
               typecode == 0x86 ||
               typecode == 0x84 ||
               typecode == 0x90 ||
               typecode == 0x8a) {
        // integer && string : type + length
        length = _data[idx++];
        unsigned int value = 0;
        ret = 0;
        while (ret < length) {
            value <<= 8;
            value += _data[ret];
            ret++;
        }
        idx += (int) length;
        xvalue_t xvalue;
        memset(&xvalue, 0, sizeof(xvalue_t));
        xvalue_set_uint(&xvalue, value);
        type_constraint(_type, &xvalue);
    } else if (typecode == 0x83 ||
               typecode == 0x91) {
        // boolean && utc : no any constraint
        if (_data[idx++] != 0x00) {
            return MMS_ERR_DATANODE;
        }
    } else if (typecode == 0xa7) {
        // float : todo:
        idx += 7;
    } else {
        // unknown type
        return MMS_ERR_DATATYPE;
    }
    return idx;
}

static int mms_type_array(
        node_t *_type,
        const unsigned char *_data,
        unsigned int _length, int _level) {
    if (_level > 9) {
        return MMS_ERR_DEPTH;
    }
    if (_type == NULL ||
        _data == NULL || _length == 0) {
        return MMS_ERR_NULL;
    }
    int idx = 0;
    unsigned int length = 0;
    // array flag
    if (_data[idx++] != 0xa1) {
        return MMS_ERR_FLAG;
    }
    int ret = mms_parse_length(
            _data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return MMS_ERR_LENGTH;
    }
    // create array
    xlist_t *type_array = xlist_create();
    if (type_array == NULL) {
        return MMS_ERR_MEMALLOC;
    }
    while (idx < _length) {
        node_t *type = node_create(NODE_TYPE_TYPE);
        if (type == NULL) {
            break;
        }
        ret = mms_type_spec(
                type, _data + idx, _level + 1);
        if (ret <= 0) {
            node_destroy(type);
            type = NULL;
            break;
        }
        idx += ret;
        xlist_append(type_array, type);
    }
    xvalue_t value;
    memset(&value, 0, sizeof(xvalue_t));
    xvalue_set_struct(&value, type_array);
    type_constraint(_type, &value);
    if (idx != _length) {
        if (ret <= 0) {
            return MMS_ERR_DATANODE;
        }
        return MMS_ERR_MEMALLOC;
    }
    return idx;
}

static void mms_varattr_response(
        response_t *_resp,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _resp;
    int code = MMS_ERR_LENGTH;
    int idx = 0;
    unsigned int length = 0;
    do {
        int ret = mms_parse_length(_data + idx, &length);
        if (ret <= 0) {
            break;
        }
        idx += ret;
        if (length != (_length - idx)) {
            break;
        }
        if (_data[idx++] != 0x80) {
            code = MMS_ERR_FLAG;
            break;
        }
        if (_data[idx++] != 0x01) {
            break;
        }
        _resp->delete = _data[idx++];
        if (_data[idx++] != 0xa2) {
            code = MMS_ERR_FLAG;
            break;
        }
        ret = mms_parse_length(_data + idx, &length);
        if (ret <= 0) {
            break;
        }
        idx += ret;
        if (length != (_length - idx)) {
            break;
        }
        if (_data[idx++] != 0xa2) {
            code = MMS_ERR_FLAG;
            break;
        }
        ret = mms_parse_length(_data + idx, &length);
        if (ret <= 0) {
            break;
        }
        idx += ret;
        if (length != (_length - idx)) {
            break;
        }
        node_t *type = node_create(NODE_TYPE_TYPE);
        if (type == NULL) {
            code = MMS_ERR_MEMALLOC;
            break;
        }
        ret = mms_type_array(
                type, _data + idx,
                length, 1);
        if (ret <= 0) {
            code = MMS_ERR_DATANODE;
            break;
        }
        idx += ret;
        _resp->data.node = type;
        service->index += idx;
        return;
    } while (0);
    service->code = code;
    service->index += idx;
}

static void mms_varattr_list_request(
        request_t *_request,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _request;
    node_t *varspec = NULL;
    int code = MMS_ERR_LENGTH;
    int idx = 0;
    unsigned int length = 0;
    do {
        int ret = mms_parse_length(
                _data + idx, &length);
        if (ret <= 0) {
            break;
        }
        idx += ret;
        if (length != (_length - idx)) {
            break;
        }
        varspec = node_create(NODE_TYPE_VARSPEC);
        if (varspec == NULL) {
            code = MMS_ERR_MEMALLOC;
            break;
        }
        ret = mms_parse_domain(_data + idx, varspec);
        if (ret < 0) {
            code = MMS_ERR_DOMAIN;
            break;
        }
        idx += ret;
        _request->data.node = varspec;
        service->index += idx;
        return;
    } while (0);
    service->code = code;
    service->index += idx;
    node_destroy(varspec);
    varspec = NULL;
}

static void mms_varattr_list_response(
        response_t *_resp,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _resp;
    int idx = 0;
    int code = MMS_ERR_LENGTH;
    do {
        unsigned int length = 0;
        int ret = mms_parse_length(
                _data + idx, &length);
        if (ret <= 0) {
            break;
        }
        idx += ret;
        if (length != (_length - idx)) {
            break;
        }
        if (_data[idx++] != 0x80) {
            code = MMS_ERR_FLAG;
            break;
        }
        if (_data[idx++] != 0x01) {
            break;
        }
        // mms deletable
        _resp->delete = _data[idx++];
        // item list flag
        if (_data[idx++] != 0xa1) {
            code = MMS_ERR_FLAG;
            break;
        }
        ret = mms_parse_length(
                _data + idx, &length);
        if (ret <= 0) {
            break;
        }
        idx += ret;
        if (length != (_length - idx)) {
            break;
        }
        xlist_t *list = xlist_create();
        if (list == NULL) {
            code = MMS_ERR_MEMALLOC;
            break;
        }
        while (idx < _length) {
            node_t *node = node_create(
                    NODE_TYPE_VARSPEC);
            if (node == NULL) {
                code = MMS_ERR_MEMALLOC;
                break;
            }
            ret = mms_var_spec(_data + idx, node);
            if (ret < 0) {
                code = ret;
                node_destroy(node);
                break;
            }
            idx += ret;
            xlist_append(list, node);
        }
        _resp->data.list = list;
        if (idx == _length) {
            service->index += idx;
            return;
        }
    } while (0);
    service->code = code;
    service->index += idx;
}

// 解析文件请求服务
static void mms_file_dir_request(
        request_t *_request,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _request;
    node_t *directory = NULL;
    int code = MMS_ERR_LENGTH;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto filedir_request_exit;
    }
    idx += ret;
    if (length != (_length - idx)) {
        goto filedir_request_exit;
    }
    // directory name list flag: 0xa0
    if (_data[idx++] != 0xa0) {
        service->code = MMS_ERR_FLAG;
        goto filedir_request_exit;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto filedir_request_exit;
    }
    idx += ret;
    if (length != (_length - idx)) {
        goto filedir_request_exit;
    }
    // filename flag: 0x19
    if (_data[idx++] != 0x19) {
        code = MMS_ERR_FLAG;
        goto filedir_request_exit;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto filedir_request_exit;
    }
    idx += ret;
    directory = node_create(NODE_TYPE_FILESPEC);
    if (directory == NULL) {
        code = MMS_ERR_MEMALLOC;
        goto filedir_request_exit;
    }
    ret = file_spec_path(directory, (char *) _data + idx, length);
    if (ret < 0) {
        code = MMS_ERR_DATANODE;
        goto filedir_request_exit;
    }
    idx += (int) length;
    if (idx == _length) {
        service->index += idx;
        _request->data.node = directory;
        return;
    }
    filedir_request_exit:
    service->code = code;
    service->index += idx;
    node_destroy(directory);
    directory = NULL;
}

// 解析目录项
static int mms_dir_entry(
        const unsigned char *_data,
        node_t *_entry) {
    if (_data == NULL || _entry == NULL) {
        return MMS_ERR_NULL;
    }
    int idx = 0;
    // directory entry flag: 0x30
    if (_data[idx++] != 0x30) {
        return MMS_ERR_FLAG;
    }
    unsigned int total_len = 0;
    unsigned int length = 0;
    int lensz = mms_parse_length(_data + idx, &length);
    if (lensz <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += lensz;
    total_len = length + idx;
    if (_data[idx++] != 0xa0) {
        return MMS_ERR_FLAG;
    }
    unsigned int path_len = 0;
    lensz = mms_parse_length(_data + idx, &length);
    if (lensz <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += lensz;
    if (length >= total_len) {
        return MMS_ERR_LENGTH;
    }
    path_len = length;
    // file name flag: 0x19
    if (_data[idx++] != 0x19) {
        return MMS_ERR_FLAG;
    }
    lensz = mms_parse_length(_data + idx, &length);
    if (lensz <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += lensz;
    if (length != (path_len - lensz - 1)) {
        return MMS_ERR_LENGTH;
    }
    dir_entry_path(_entry, (char *) _data + idx, length);
    idx += (int) length;
    if (_data[idx++] != 0xa1) {
        return MMS_ERR_FLAG;
    }
    lensz = mms_parse_length(_data + idx, &length);
    if (lensz <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += lensz;
    if (length != (total_len - idx)) {
        return MMS_ERR_LENGTH;
    }
    if (_data[idx++] != 0x80) {
        return MMS_ERR_FLAG;
    }
    {
        int len = _data[idx++];
        int dataidx = 0;
        unsigned int value = 0;
        while (dataidx < len) {
            value <<= 8;
            value += _data[idx++];
            dataidx++;
        }
        dir_entry_size(_entry, value);
    }
    if (_data[idx++] != 0x81) {
        return MMS_ERR_FLAG;
    }
    if (_data[idx++] != 0x0f) {
        return MMS_ERR_LENGTH;
    }
    dir_entry_stamp(_entry, (char *) (_data + idx));
    idx += 0x0f;
    return idx;
}

// 解析目录项服务响应报文
static void mms_file_dir_response(
        response_t *_resp,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _resp;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    // directory entry list flag: 0xa0
    if (_data[idx++] != 0xa0) {
        service->code = MMS_ERR_FLAG;
        service->index += idx;
        return;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    unsigned int size = _data[idx++];
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        service->code = MMS_ERR_LENGTH;
        service->index += idx;
        return;
    }
    xlist_t *list = xlist_create();
    if (list == NULL) {
        service->code = MMS_ERR_MEMALLOC;
        service->index += idx;
        return;
    }
    while (idx < _length) {
        node_t *entry = node_create(NODE_TYPE_DIRENTRY);
        if (entry == NULL) {
            break;
        }
        ret = mms_dir_entry(_data + idx, entry);
        if (ret <= 0) {
            node_destroy(entry);
            entry = NULL;
            break;
        }
        idx += ret;
        xlist_append(list, entry);
    }
    if (xlist_count(list) != size) {
        xlist_destroy(list);
        list = NULL;
    }
    service->index += idx;
    _resp->data.list = list;
}

static void mms_fopen_request(
        request_t *_request,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _request;
    node_t *req = NULL;
    int code = MMS_ERR_LENGTH;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto fopen_exit;
    }
    idx += ret;
    if (length != (_length - idx)) {
        goto fopen_exit;
    }
    // file name list flag: 0xa0
    if (_data[idx++] != 0xa0) {
        code = MMS_ERR_FLAG;
        goto fopen_exit;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto fopen_exit;
    }
    idx += ret;
    if (_data[length + idx] != 0x81) {
        goto fopen_exit;
    }
    // filename flag: 0x19
    if (_data[idx++] != 0x19) {
        // log: unknown file name flag
        code = MMS_ERR_FLAG;
        goto fopen_exit;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto fopen_exit;
    }
    idx += ret;
    req = node_create(NODE_TYPE_FOPENREQ);
    if (req == NULL) {
        code = MMS_ERR_MEMALLOC;
        goto fopen_exit;
    }
    fopen_req_path(req, (char *) _data + idx, length);
    idx += (int) length;
    if (_data[idx++] != 0x81) {
        code = MMS_ERR_FLAG;
        goto fopen_exit;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto fopen_exit;
    }
    idx += ret;
    if (length > sizeof(unsigned int)) {
        goto fopen_exit;
    }
    unsigned int int_idx = 0;
    unsigned int pos = 0;
    while (int_idx < length) {
        pos <<= 8;
        pos += _data[idx + int_idx];
        int_idx++;
    }
    idx += (int) length;
    fopen_req_position(req, pos);
    _request->data.node = req;
    service->index += idx;
    return; // success
    fopen_exit: // exit work
    service->code = code;
    service->index += idx;
    node_destroy(req);
    req = NULL;
}

static void mms_fopen_response(
        response_t *_resp,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _resp;
    node_t *resp = NULL;
    int code = MMS_ERR_LENGTH;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto fopen_resp_exit;
    }
    idx += ret;
    if (length != (_length - idx)) {
        goto fopen_resp_exit;
    }
    if (_data[idx++] != 0x80) {
        code = MMS_ERR_FLAG;
        goto fopen_resp_exit;
    }
    length = _data[idx++];
    if (length > sizeof(unsigned int)) {
        goto fopen_resp_exit;
    }
    resp = node_create(NODE_TYPE_FOPENRESP);
    if (resp == NULL) {
        code = MMS_ERR_MEMALLOC;
        goto fopen_resp_exit;
    }
    // frsm
    unsigned int int_idx = 0;
    unsigned int frsm = 0;
    while (int_idx < length) {
        frsm <<= 8;
        frsm += _data[idx + int_idx];
        int_idx++;
    }
    idx += (int) length;
    fopen_resp_frsm(resp, frsm);
    // file attr
    if (_data[idx++] != 0xa1) {
        code = MMS_ERR_FLAG;
        goto fopen_resp_exit;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto fopen_resp_exit;
    }
    idx += ret;
    if (length != (_length - idx)) {
        goto fopen_resp_exit;
    }
    // file attr.size
    if (_data[idx++] != 0x80) {
        code = MMS_ERR_FLAG;
        goto fopen_resp_exit;
    }
    length = _data[idx++];
    if (length > sizeof(size_t)) {
        goto fopen_resp_exit;
    }
    int_idx = 0;
    size_t fsize = 0;
    while (int_idx < length) {
        fsize <<= 8;
        fsize += _data[idx + int_idx];
        int_idx++;
    }
    idx += (int) length;
    // file attr.stamp
    if (_data[idx++] != 0x81) {
        code = MMS_ERR_FLAG;
        goto fopen_resp_exit;
    }
    if (_data[idx++] != 0x0f) {
        goto fopen_resp_exit;
    }
    fopen_resp_attr(resp, fsize, (char *) _data + idx);
    idx += 0x0f;
    _resp->data.node = resp;
    service->index += idx;
    return;
    fopen_resp_exit: // exit work
    service->code = code;
    service->index += idx;
    node_destroy(resp);
    resp = NULL;
}

static void mms_fread_request(
        request_t *_request,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _request;
    node_t *req = NULL;
    int code = MMS_ERR_LENGTH;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto fread_request_exit;
    }
    idx += ret;
    if (length != (_length - idx)) {
        goto fread_request_exit;
    }
    if (length > sizeof(unsigned int)) {
        goto fread_request_exit;
    }
    unsigned int int_idx = 0;
    unsigned int value = 0;
    while (int_idx < length) {
        value <<= 8;
        value += _data[idx + int_idx];
        int_idx++;
    }
    idx += (int) length;
    req = node_create(NODE_TYPE_FREAD);
    if (req == NULL) {
        code = MMS_ERR_MEMALLOC;
        goto fread_request_exit;
    }
    fread_value(req, value);
    _request->data.node = req;
    service->index += idx;
    return;
    fread_request_exit:
    service->code = code;
    service->index += idx;
    node_destroy(req);
    req = NULL;
}

static void mms_fread_response(
        response_t *_resp,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _resp;
    node_t *resp = NULL;
    int code = MMS_ERR_LENGTH;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto fread_resp_exit;
    }
    idx += ret;
    if (length != (_length - idx)) {
        goto fread_resp_exit;
    }
    if (_data[idx++] != 0x80) {
        code = MMS_ERR_FLAG;
        goto fread_resp_exit;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto fread_resp_exit;
    }
    idx += ret;
    if (length != (_length - idx) &&
        _data[length + idx] != 0x81) {
        goto fread_resp_exit;
    }
    resp = node_create(NODE_TYPE_FREADRESP);
    if (resp == NULL) {
        code = MMS_ERR_MEMALLOC;
        goto fread_resp_exit;
    }
    fread_resp_size(resp, length);
    if (length > 0) {
        fread_resp_flag(resp, _data + idx, length, 1);
    }
    if (length > 8) {
        fread_resp_flag(resp, _data + idx + length - 4, 4, 0);
    } else if (length > 4) {
        fread_resp_flag(resp, _data + idx + 4, length - 4, 0);
    }
    idx += (int) length;
    if (idx == _length) {
        fread_resp_follow(resp, 1);
        _resp->data.node = resp;
        service->index += idx;
        return;
    }
    if (_data[idx++] != 0x81) {
        code = MMS_ERR_FLAG;
        goto fread_resp_exit;
    }
    if (_data[idx++] != 0x01) {
        goto fread_resp_exit;
    }
    if (_data[idx++] != 0x00) {
        code = MMS_ERR_FLAG;
        goto fread_resp_exit;
    }
    fread_resp_follow(resp, 0);
    _resp->data.node = resp;
    service->index += idx;
    return;
    fread_resp_exit:
    service->code = code;
    service->index += idx;
    node_destroy(resp);
}

static void mms_fclose_request(
        request_t *_request,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _request;
    node_t *fclose1 = NULL;
    int code = MMS_ERR_LENGTH;
    int idx = 0;
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        goto fclose_req_exit;
    }
    idx += ret;
    if (length != (_length - idx)) {
        goto fclose_req_exit;
    }
    if (length > sizeof(unsigned int)) {
        goto fclose_req_exit;
    }
    unsigned int int_idx = 0;
    unsigned int value = 0;
    while (int_idx < length) {
        value <<= 8;
        value += _data[idx + int_idx];
        int_idx++;
    }
    idx += (int) length;
    fclose1 = node_create(NODE_TYPE_FCLOSE);
    if (fclose1 == NULL) {
        code = MMS_ERR_MEMALLOC;
        goto fclose_req_exit;
    }
    fclose_value(fclose1, 1, value);
    _request->data.node = fclose1;
    service->index += idx;
    return;
    fclose_req_exit:
    service->code = code;
    service->index += idx;
    node_destroy(fclose1);
    fclose1 = NULL;
}

static void mms_fclose_response(
        response_t *_resp,
        const unsigned char *_data,
        size_t _length) {
    service_t *service = (service_t *) _resp;
    node_t *fclose1 = NULL;
    int idx = 0;
    int code = MMS_ERR_NULL;
    if (_data[idx++] != 0x00) {
        code = MMS_ERR_FLAG;
        goto fclose_resp_exit;
    }
    fclose1 = node_create(NODE_TYPE_FCLOSE);
    if (fclose1 == NULL) {
        goto fclose_resp_exit;
    }
    fclose_value(fclose1, 0, 0);
    _resp->data.node = fclose1;
    service->index += idx;
    return;
    fclose_resp_exit:
    service->code = code;
    service->index += idx;
    node_destroy(fclose1);
    fclose1 = NULL;
}

/***************************************to_string***************************************/

static int list_tostring(
        xlist_t *_list, char *_dest, size_t _size,
        const char *_header) {
    if (_list == NULL) {
        return 0;
    }
    int idx = 0;
    if (_header != NULL && _header[0] != 0) {
        const char *data = xtrans(_header);
        int ret = (int) strlen(data);
        if (ret >= _size) {
            return 0;
        }
        strcpy(_dest + idx, data);
        idx += ret;
    }
    node_t *node = xlist_begin(_list);
    while (node && idx < (_size - 1)) {
        _dest[idx++] = '\n';
        int ret = node_tostring(
                node, _dest + idx, _size - idx);
        if (ret < 0) {
            _dest[idx] = 0;
            break;
        }
        idx += ret;
        node = xlist_next(_list);
    }
    if (idx >= (_size - 1)) {
        idx = (int) _size - 1;
        return idx;
    }
    _dest[idx++] = '\n';
    _dest[idx] = 0;
    return idx;
}

static int node_request_tostring(
        const request_t *_req,
        char *_dest, size_t _size) {
    return node_tostring(_req->data.node, _dest, _size);
}

static int varattr_request_tostring(
        const request_t *_req,
        char *_dest, size_t _size) {
    int idx = 0;
    const char *header = xtrans("varAccessAttributes:{");
    int ret = (int) strlen(header);
    if (ret >= (_size - 1)) {
        return 0;
    }
    strcpy(_dest, header);
    idx += ret;
    ret = node_tostring(
            _req->data.node,
            _dest + idx, _size - idx);
    if (ret < 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static int filedir_request_tostring(
        const request_t *_req,
        char *_dest, size_t _size) {
    int idx = 0;
    const char *data = xtrans("fileDirRequest:{");
    int ret = (int) strlen(data);
    if (ret >= _size) {
        return 0;
    }
    strcpy(_dest + idx, data);
    idx += ret;
    ret = node_tostring(
            _req->data.node,
            _dest + idx, _size - idx);
    if (ret < 0) {
        return ret;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static int read_request_tostring(
        const request_t *_req,
        char *_dest, size_t _size) {
    int ret = list_tostring(
            _req->data.list, _dest,
            _size, "readVarRequest:{");
    if (ret <= 0) {
        _dest[0] = 0;
        return ret;
    }
    if (ret < (_size - 1)) {
        _dest[ret++] = '}';
        _dest[ret] = 0;
    }
    return ret;
}

static int writ_request_tostring(
        const request_t *_req,
        char *_dest, size_t _size) {
    int ret = list_tostring(
            _req->data.list, _dest,
            _size, "writeVarRequest:{");
    if (ret < 0) {
        _dest[0] = 0;
        return ret;
    }
    if (ret < (_size - 1)) {
        _dest[ret++] = '}';
        _dest[ret] = 0;
    }
    return ret;
}

static int names_request_tostring(
        const request_t *_req,
        char *_dest, size_t _size) {
    int idx = 0;
    const char *header = xtrans("getNamesRequest:{");
    int ret = (int) strlen(header);
    if (ret >= (_size - 1)) {
        return 0;
    }
    strcpy(_dest, header);
    idx += ret;
    ret = node_tostring(
            _req->data.node,
            _dest + idx, _size - idx);
    if (ret < 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static int varattr_response_tostring(
        const response_t *_resp,
        char *_dest, size_t _size) {
    xlist_t *list =
            type_get_constraint(
                    _resp->data.node);
    if (list == NULL) {
        return MMS_ERR_NULL;
    }
    int ret = list_tostring(
            list, _dest, _size,
            "varAccessAttributes:{");
    if (ret < 0) {
        _dest[0] = 0;
        return ret;
    }
    if (ret < (_size - 1)) {
        const char *fmt = xtrans("deletable:%s\n");
        const char *data = xtrans("false");
        if (_resp->delete) {
            data = xtrans("true");
        }
        int len = snprintf(
                _dest + ret,
                _size - ret - 1, fmt, data);
        if (len < 0) {
            return MMS_ERR_LENGTH;
        }
        ret += len;
    }
    if (ret < (_size - 1)) {
        _dest[ret++] = '}';
        _dest[ret] = 0;
    }
    return ret;
}

static int varattrs_request_tostring(
        const request_t *_req,
        char *_dest, size_t _size) {
    int idx = 0;
    const char *header = xtrans("getVarListAttrRequest:{");
    int ret = (int) strlen(header);
    if (ret >= (_size - 1)) {
        return 0;
    }
    strcpy(_dest, header);
    idx += ret;
    ret = node_tostring(
            _req->data.node,
            _dest + idx, _size - idx);
    if (ret < 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (idx >= (_size - 1)) {
        return idx;
    }
    _dest[idx++] = '}';
    _dest[idx] = 0;
    return idx;
}

static int node_response_tostring(
        const response_t *_resp,
        char *_dest, size_t _size) {
    return node_tostring(_resp->data.node, _dest, _size);
}

static int names_response_tostring(
        const response_t *_resp,
        char *_dest, size_t _size) {
    int ret = list_tostring(
            _resp->data.list, _dest,
            _size, "getNamesResp:{");
    if (ret < 0) {
        _dest[0] = 0;
        return ret;
    }
    if (_resp->follow_has && ret < (_size - 1)) {
        const char *fmt = xtrans("follow:%s\n");
        const char *data = xtrans("false");
        if (_resp->follow_is) {
            data = xtrans("true");
        }
        int len = snprintf(
                _dest + ret,
                _size - ret, fmt, data);
        if (len < 0) {
            return MMS_ERR_LENGTH;
        }
        ret += len;
    }
    if (ret < (_size - 1)) {
        _dest[ret++] = '}';
        _dest[ret] = 0;
    }
    return ret;
}

static int varattrs_response_tostring(
        const response_t *_resp,
        char *_dest, size_t _size) {
    int ret = list_tostring(
            _resp->data.list, _dest,
            _size, "getVarListAttrResp:{");
    if (ret < 0) {
        _dest[0] = 0;
        return ret;
    }
    if (ret < (_size - 1)) {
        const char *fmt = xtrans("deletable:%s\n");
        const char *data = xtrans("false");
        if (_resp->delete) {
            data = xtrans("true");
        }
        int len = snprintf(
                _dest + ret,
                _size - ret - 1, fmt, data);
        if (len < 0) {
            return MMS_ERR_LENGTH;
        }
        ret += len;
    }
    if (ret < (_size - 1)) {
        _dest[ret++] = '}';
        _dest[ret] = 0;
    }
    return ret;
}

static int filedir_response_tostring(
        const response_t *_resp,
        char *_dest, size_t _size) {
    int ret = list_tostring(
            _resp->data.list, _dest,
            _size, "fileDirResponse:{");
    if (ret <= 0) {
        _dest[0] = 0;
        return ret;
    }
    if (ret < (_size - 1)) {
        _dest[ret++] = '}';
        _dest[ret] = 0;
    }
    return ret;
}

static int read_response_tostring(
        const response_t *_resp,
        char *_dest, size_t _size) {
    int ret = list_tostring(
            _resp->data.list, _dest,
            _size, "readVarResponse:{");
    if (ret <= 0) {
        _dest[0] = 0;
        return ret;
    }
    if (ret < (_size - 1)) {
        _dest[ret++] = '}';
        _dest[ret] = 0;
    }
    return ret;
}

static int writ_response_tostring(
        const response_t *_resp,
        char *_dest, size_t _size) {
    int ret = list_tostring(
            _resp->data.list, _dest,
            _size, "writeVarResp:{");
    if (ret <= 0) {
        _dest[0] = 0;
        return ret;
    }
    if (ret < (_size - 1)) {
        _dest[ret++] = '}';
        _dest[ret] = 0;
    }
    return ret;
}

static int init_tostring(
        const service_t *_service,
        char *_dest, size_t _size) {
    if (_service == NULL ||
        _dest == NULL || _size == 0) {
        return MMS_ERR_NULL;
    }
    if (_service->type != MMS_MSG_INIT_REQ &&
        _service->type != MMS_MSG_INIT_RESP) {
        return MMS_ERR_MSGTYPE;
    }
    const initdata_t *init = (initdata_t *) _service;
    return node_tostring(init->data, _dest, _size);
}

typedef struct req_tostr_t {
    int req;

    int (*tostring)(const request_t *, char *, size_t);
} req_tostr_t;

static int request_tostring(
        const service_t *_service,
        char *_dest, size_t _size) {
    static const req_tostr_t g_req2str[] = {
            {MMS_SERVICE_VARATTR, varattr_request_tostring},
            {MMS_SERVICE_VARIDX,  varattrs_request_tostring},
            {MMS_SERVICE_NAMES,   names_request_tostring},
            {MMS_SERVICE_WRITE,   writ_request_tostring},
            {MMS_SERVICE_READ,    read_request_tostring},
            {MMS_SERVICE_FILEDIR, filedir_request_tostring},
            {MMS_SERVICE_FOPEN,   node_request_tostring},
            {MMS_SERVICE_FREAD,   node_request_tostring},
            {MMS_SERVICE_FCLOSE,  node_request_tostring},
            {0, NULL},
    };
    if (_service == NULL ||
        _dest == NULL || _size == 0) {
        return MMS_ERR_NULL;
    }
    if (_service->type != MMS_MSG_REQUEST) {
        return MMS_ERR_MSGTYPE;
    }
    request_t *req = (request_t *) _service;
    const req_tostr_t *req2str = g_req2str;
    while (req2str->tostring) {
        if (req2str->req == req->type) {
            break;
        }
        req2str++;
    }
    if (req2str->tostring == NULL) {
        return MMS_ERR_REQTYPE;
    }
    return req2str->tostring(req, _dest, _size);
}

typedef struct resp_tostr_t {
    int resp;

    int (*tostring)(const response_t *, char *, size_t);
} resp_tostr_t;

static int response_tostring(
        const service_t *_service,
        char *_dest, size_t _size) {
    static const resp_tostr_t g_resp2str[] = {
            {MMS_SERVICE_VARATTR, varattr_response_tostring},
            {MMS_SERVICE_VARIDX,  varattrs_response_tostring},
            {MMS_SERVICE_NAMES,   names_response_tostring},
            {MMS_SERVICE_WRITE,   writ_response_tostring},
            {MMS_SERVICE_READ,    read_response_tostring},
            {MMS_SERVICE_FILEDIR, filedir_response_tostring},
            {MMS_SERVICE_FOPEN,   node_response_tostring},
            {MMS_SERVICE_FREAD,   node_response_tostring},
            {MMS_SERVICE_FCLOSE,  node_response_tostring},
            {0, NULL},
    };
    if (_service == NULL ||
        _dest == NULL || _size == 0) {
        return MMS_ERR_NULL;
    }
    if (_service->type != MMS_MSG_RESPONSE) {
        return MMS_ERR_MSGTYPE;
    }
    response_t *resp = (response_t *) _service;
    const resp_tostr_t *resp2str = g_resp2str;
    while (resp2str->tostring) {
        if (resp2str->resp == resp->type) {
            break;
        }
        resp2str++;
    }
    if (resp2str->tostring == NULL) {
        return MMS_ERR_RESPTYPE;
    }
    return resp2str->tostring(resp, _dest, _size);
}

static int report_tostring(
        const service_t *_service,
        char *_dest, size_t _size) {
    if (_service == NULL ||
        _dest == NULL || _size == 0) {
        return MMS_ERR_NULL;
    }
    if (_service->type != MMS_MSG_REPORT) {
        return MMS_ERR_MSGTYPE;
    }
    report_t *report = (report_t *) _service;
    int ret = list_tostring(
            report->data, _dest,
            _size, "infoReport:{");
    if (ret <= 0) {
        _dest[0] = 0;
        return ret;
    }
    if (ret < (_size - 1)) {
        _dest[ret++] = '}';
        _dest[ret] = 0;
    }
    return ret;
}

static int init_destroy(service_t *_service) {
    if (_service == NULL) {
        return 0;
    }
    if (_service->type != MMS_MSG_INIT_REQ &&
        _service->type != MMS_MSG_INIT_RESP) {
        return MMS_ERR_MSGTYPE;
    }
    initdata_t *init = (initdata_t *) _service;
    node_destroy(init->data);
    init->data = NULL;
    free(_service);
    _service = NULL;
    return 0;
}

static int report_destroy(service_t *_service) {
    if (_service == NULL) {
        return 0;
    }
    if (_service->type != MMS_MSG_REPORT) {
        return MMS_ERR_MSGTYPE;
    }
    report_t *report = (report_t *) _service;
    xlist_destroy(report->data);
    report->data = NULL;
    free(_service);
    _service = NULL;
    return 0;
}

static int request_destroy(service_t *_service) {
    if (_service == NULL) {
        return 0;
    }
    if (_service->type != MMS_MSG_REQUEST) {
        return MMS_ERR_MSGTYPE;
    }
    request_t *request = (request_t *) _service;
    if (request->type == MMS_SERVICE_VARIDX ||
        request->type == MMS_SERVICE_VARATTR ||
        request->type == MMS_SERVICE_NAMES ||
        request->type == MMS_SERVICE_FILEDIR ||
        request->type == MMS_SERVICE_FOPEN ||
        request->type == MMS_SERVICE_FREAD ||
        request->type == MMS_SERVICE_FCLOSE) {
        node_destroy(request->data.node);
        request->data.node = NULL;
    } else if (request->type == MMS_SERVICE_WRITE ||
               request->type == MMS_SERVICE_READ) {
        xlist_destroy(request->data.list);
        request->data.list = NULL;
    } else {
        return MMS_ERR_REQTYPE;
    }
    free(_service);
    _service = NULL;
    return 0;
}

static int response_destroy(service_t *_service) {
    if (_service == NULL) {
        return 0;
    }
    if (_service->type != MMS_MSG_RESPONSE) {
        return MMS_ERR_MSGTYPE;
    }
    response_t *resp = (response_t *) _service;
    if (resp->type == MMS_SERVICE_READ ||
        resp->type == MMS_SERVICE_WRITE ||
        resp->type == MMS_SERVICE_NAMES ||
        resp->type == MMS_SERVICE_FILEDIR ||
        resp->type == MMS_SERVICE_VARIDX) {
        xlist_destroy(resp->data.list);
        resp->data.list = NULL;
    } else if (resp->type == MMS_SERVICE_VARATTR ||
               resp->type == MMS_SERVICE_FOPEN ||
               resp->type == MMS_SERVICE_FREAD ||
               resp->type == MMS_SERVICE_FCLOSE) {
        node_destroy(resp->data.node);
        resp->data.node = NULL;
    } else {
        return MMS_ERR_RESPTYPE;
    }
    free(_service);
    _service = NULL;
    return 0;
}

static void mms_parse_initdata(
        service_t *_service,
        const unsigned char *_data,
        size_t _length) {
    node_t *data = NULL;
    int idx = 0;
    int ret = _data[idx++];
    if ((ret != MMS_MSG_INIT_REQ ||
         _service->type != MMS_MSG_INIT_REQ) &&
        (ret != MMS_MSG_INIT_RESP ||
         _service->type != MMS_MSG_INIT_RESP)) {
        _service->code = MMS_ERR_MSGTYPE;
        _service->index = idx;
        return;
    }
    initdata_t *init = (initdata_t *) _service;
    data = node_create(NODE_TYPE_INIT);
    if (data == NULL) {
        _service->code = MMS_ERR_MEMALLOC;
        _service->index = idx;
        return;
    }
    unsigned int length = 0;
    int code = MMS_ERR_LENGTH;
    do {
        ret = mms_parse_length(_data + idx, &length);
        if (ret < 0) {
            break;
        }
        idx += ret;
        if (length != (_length - idx)) {
            break;
        }
        // local detail calling flag
        if (_data[idx++] != 0x80) {
            code = MMS_ERR_FLAG;
            break;
        }
        length = _data[idx++];
        if (length > sizeof(unsigned int)) {
            break;
        }
        ret = 0;
        unsigned int value = 0;
        while (ret < length) {
            value <<= 8;
            value += _data[idx + ret];
            ret++;
        }
        idx += ret;
        init_detail_called(data, value);
        // max serv outstanding calling flag
        if (_data[idx++] != 0x81) {
            code = MMS_ERR_FLAG;
            break;
        }
        if (_data[idx++] != 0x01) {
            break;
        }
        value = _data[idx++];
        init_max_calling(data, value);
        // max serv outstanding called flag
        if (_data[idx++] != 0x82) {
            code = MMS_ERR_FLAG;
            break;
        }
        if (_data[idx++] != 0x01) {
            break;
        }
        value = _data[idx++];
        init_max_called(data, value);
        // data structure nesting level
        if (_data[idx++] != 0x83) {
            code = MMS_ERR_FLAG;
            break;
        }
        if (_data[idx++] != 0x01) {
            code = MMS_ERR_LENGTH;
            break;
        }
        value = _data[idx++];
        init_struct_nest(data, value);
        // init detail
        if (_data[idx++] != 0xa4) {
            code = MMS_ERR_FLAG;
            break;
        }
        ret = mms_parse_length(_data + idx, &length);
        if (ret <= 0) {
            break;
        }
        idx += ret;
        if (length != (_length - idx)) {
            break;
        }
        if (_data[idx++] != 0x80) {
            code = MMS_ERR_FLAG;
            break;
        }
        if (_data[idx++] != 0x01) {
            break;
        }
        value = _data[idx++];
        init_version(data, value);
        if (_data[idx++] != 0x81) {
            code = MMS_ERR_FLAG;
            break;
        }
        if (_data[idx++] != 0x03) {
            break;
        }
        init_param_cbb(data, _data + idx);
        idx += 0x03;
        if (_data[idx++] != 0x82) {
            code = MMS_ERR_FLAG;
            break;
        }
        if (_data[idx++] != 0x0c) {
            break;
        }
        init_services(data, _data + idx);
        idx += 0x0c;
        _service->index = idx;
        init->data = data;
        return;
    } while (0);
    _service->code = code;
    _service->index = idx;
    node_destroy(data);
    data = NULL;
}

static void mms_parse_report(
        service_t *_service,
        const unsigned char *_data,
        size_t _length) {
    int idx = 0;
    if (_data[idx++] != MMS_MSG_REPORT ||
        _service->type != MMS_MSG_REPORT) {
        _service->code = MMS_ERR_MSGTYPE;
        _service->index = idx;
        return;
    }
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        _service->code = MMS_ERR_LENGTH;
        _service->index = idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        _service->code = MMS_ERR_LENGTH;
        _service->index = idx;
        return;
    }
    if (_data[idx++] != 0xa0) {
        _service->code = MMS_ERR_FLAG;
        _service->index = idx;
        return;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        _service->code = MMS_ERR_LENGTH;
        _service->index = idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        _service->code = MMS_ERR_LENGTH;
        _service->index = idx;
        return;
    }
    {
        unsigned char flags[] = {
                0xa1, 0x05, 0x80,
                0x03, 0x52, 0x50, 0x54
        };
        int fsize = sizeof(flags);
        int fidx = 0;
        while (fidx < fsize) {
            if (_data[idx++] != flags[fidx]) {
                _service->code = MMS_ERR_FLAG;
                _service->index = idx;
                return;
            }
            fidx++;
        }
    }
    // report list flag
    if (_data[idx++] != 0xa0) {
        _service->code = MMS_ERR_FLAG;
        _service->index = idx;
        return;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        _service->code = MMS_ERR_LENGTH;
        _service->index = idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        _service->code = MMS_ERR_LENGTH;
        _service->index = idx;
        return;
    }
    report_t *report = (report_t *) _service;
    if (report->data == NULL) {
        report->data = xlist_create();
    }
    if (report->data == NULL) {
        _service->code = MMS_ERR_MEMALLOC;
        _service->index = idx;
        return;
    }
    while (idx < _length) {
        node_t *var = node_create(NODE_TYPE_UDATA);
        if (var == NULL) {
            break;
        }
        xvalue_t value;
        memset(&value, 0, sizeof(xvalue_t));
        ret = mms_data_value(_data + idx, &value, 1);
        if (ret <= 0) {
            node_destroy(var);
            var = NULL;
            _service->code = MMS_ERR_FLAG;
            break;
        }
        idx += ret;
        udata_value(var, &value);
        xlist_append(report->data, var);
    }
    _service->index = idx;
}

typedef struct reqfunc_t {
    int req;

    void (*func)(request_t *, const unsigned char *, size_t);
} reqfunc_t;

static void mms_parse_request(
        service_t *_service,
        const unsigned char *_data,
        size_t _length) {
    static const reqfunc_t g_reqfunc[] = {
            {MMS_SERVICE_NAMES,   mms_getnamelist_request},
            {MMS_SERVICE_VARATTR, mms_varattr_request},
            {MMS_SERVICE_VARIDX,  mms_varattr_list_request},
            {MMS_SERVICE_WRITE,   mms_write_request},
            {MMS_SERVICE_READ,    mms_read_request},
            {MMS_SERVICE_FILEDIR, mms_file_dir_request},
            {MMS_SERVICE_FOPEN,   mms_fopen_request},
            {MMS_SERVICE_FREAD,   mms_fread_request},
            {MMS_SERVICE_FCLOSE,  mms_fclose_request},
            {0,                   0},
    };
    int idx = 0;
    if (_data[idx++] != MMS_MSG_REQUEST ||
        _service->type != MMS_MSG_REQUEST) {
        // log::error not request message
        _service->code = MMS_ERR_MSGTYPE;
        _service->index = idx;
        return;
    }
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        // log::error request length
        _service->code = MMS_ERR_LENGTH;
        _service->index = idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        // log::error request length
        _service->code = MMS_ERR_LENGTH;
        _service->index = idx;
        return;
    }
    unsigned int invoke = 0;
    ret = mms_parse_invoke(_data + idx, &invoke);
    if (ret <= 0) {
        // log::error invoke id
        _service->code = MMS_ERR_INVOKE;
        _service->index = idx;
    }
    idx += ret;
    request_t *request = (request_t *) _service;
    request->invoke = invoke;
    if (_data[idx] == 0xbf ||
        _data[idx] == 0x9f) {
        idx++;
    }
    const reqfunc_t *reqfunc = g_reqfunc;
    int reqcode = _data[idx++];
    while (reqfunc->func) {
        if (reqfunc->req == reqcode) {
            break;
        }
        reqfunc++;
    }
    _service->index = idx;
    if (reqfunc->func == NULL) {
        // log::error unknown request type
        _service->code = MMS_ERR_REQTYPE;
        return;
    }
    request->type = reqcode;
    reqfunc->func(request, _data + idx, _length - idx);
}

typedef struct respfunc_t {
    int resp;

    void (*func)(response_t *, const unsigned char *, size_t);
} respfunc_t;

static void mms_parse_response(
        service_t *_service,
        const unsigned char *_data,
        size_t _length) {
    static const respfunc_t g_respfunc[] = {
            {MMS_SERVICE_NAMES,   mms_getnamelist_response},
            {MMS_SERVICE_VARATTR, mms_varattr_response},
            {MMS_SERVICE_VARIDX,  mms_varattr_list_response},
            {MMS_SERVICE_WRITE,   mms_write_response},
            {MMS_SERVICE_READ,    mms_read_response},
            {MMS_SERVICE_FILEDIR, mms_file_dir_response},
            {MMS_SERVICE_FOPEN,   mms_fopen_response},
            {MMS_SERVICE_FREAD,   mms_fread_response},
            {MMS_SERVICE_FCLOSE,  mms_fclose_response},
            {0,                   0},
    };
    int idx = 0;
    if (_data[idx++] != MMS_MSG_RESPONSE) {
        // log::error not request message
        _service->code = MMS_ERR_MSGTYPE;
        _service->index = idx;
        return;
    }
    int ret = 0;
    unsigned int length = 0;
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        // log::error request length
        _service->code = MMS_ERR_LENGTH;
        _service->index = idx;
        return;
    }
    idx += ret;
    if (length != (_length - idx)) {
        // log::error request length
        _service->code = MMS_ERR_LENGTH;
        _service->index = idx;
        return;
    }
    unsigned int invoke = 0;
    ret = mms_parse_invoke(_data + idx, &invoke);
    if (ret <= 0) {
        // log::error invoke id
        _service->code = MMS_ERR_INVOKE;
        _service->index = idx;
    }
    idx += ret;
    response_t *resp = (response_t *) _service;
    resp->invoke = invoke;
    if (_data[idx] == 0xbf ||
        _data[idx] == 0x9f) {
        idx++;
    }
    const respfunc_t *respfunc = g_respfunc;
    int respcode = _data[idx++];
    while (respfunc->func) {
        if (respfunc->resp == respcode) {
            break;
        }
        respfunc++;
    }
    _service->index = idx;
    if (respfunc->func == NULL) {
        // log::error unknown response type
        _service->code = MMS_ERR_RESPTYPE;
        return;
    }
    resp->type = respcode;
    respfunc->func(resp, _data + idx, _length - idx);
}

service_t *mms_parse(
        const unsigned char *_data,
        size_t _length) {
    if (_data == NULL || _length == 0) {
        return NULL;
    }
    service_t *service = NULL;
    switch (_data[0]) {
        case MMS_MSG_REQUEST: {
            static const service_op_t sop = {
                    request_destroy,
                    request_tostring,
            };
            service = (service_t *) malloc(sizeof(request_t));
            if (service == NULL) {
                break;
            }
            memset(service, 0, sizeof(request_t));
            service->type = MMS_MSG_REQUEST;
            service->op = &sop;
            mms_parse_request(service, _data, _length);
            break;
        }
        case MMS_MSG_RESPONSE: {
            static const service_op_t sop = {
                    response_destroy,
                    response_tostring,
            };
            service = (service_t *) malloc(sizeof(response_t));
            if (service == NULL) {
                break;
            }
            memset(service, 0, sizeof(response_t));
            service->type = MMS_MSG_RESPONSE;
            service->op = &sop;
            mms_parse_response(service, _data, _length);
            break;
        }
        case MMS_MSG_REPORT: {
            static const service_op_t sop = {
                    report_destroy,
                    report_tostring,
            };
            service = (service_t *) malloc(sizeof(report_t));
            if (service == NULL) {
                break;
            }
            memset(service, 0, sizeof(report_t));
            service->type = MMS_MSG_REPORT;
            service->op = &sop;
            mms_parse_report(service, _data, _length);
            break;
        }
        case MMS_MSG_INIT_REQ:
        case MMS_MSG_INIT_RESP: {
            static const service_op_t sop = {
                    init_destroy,
                    init_tostring,
            };
            service = (service_t *) malloc(sizeof(initdata_t));
            if (service == NULL) {
                break;
            }
            memset(service, 0, sizeof(initdata_t));
            service->type = _data[0];
            service->op = &sop;
            mms_parse_initdata(service, _data, _length);
            break;
        }
        default: {
            // log::warn unknown message type
            service = (service_t *) malloc(sizeof(service_t));
            if (service == NULL) {
                break;
            }
            memset(service, 0, sizeof(service_t));
            service->code = MMS_ERR_MSGTYPE;
            break;
        }
    }
    return service;
}



