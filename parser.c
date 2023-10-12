
#include "parser.h"
#include <string.h>
#include <stdlib.h>

#define MMS_ERR_NULL (-1)
#define MMS_ERR_FLAG (-2)
#define MMS_ERR_LENGTH (-3)
#define MMS_ERR_DATATYPE (-4)

#define MMS_DIR_UP (0xa0)
#define MMS_DIR_DOWN (0xa1)
#define MMS_INVOKE_ID (0x02)

#define MMS_SERVICE_FILE (0x4d)
#define MMS_SERVICE_WRITE (0xa5)
#define MMS_SERVICE_READ (0xa4)

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

// 解析文件请求服务
static int mms_file_dir_request(
        const unsigned char *_data,
        size_t _length) {
    if (_data == NULL || _length == 0) {
        return -1;
    }
    int idx = 0;
    if (_data[idx++] != 0xbf) {
        return idx;
    }
    if (_data[idx++] != MMS_SERVICE_FILE) {
        return idx;
    }
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return idx;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return idx;
    }
    // file name list flag: 0xa0
    if (_data[idx++] != 0xa0) {
        // log: unknown list flag
        return idx;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return idx;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return idx;
    }
    // filename flag: 0x19
    if (_data[idx++] != 0x19) {
        // log: unknown file name flag
        return -6;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return -7;
    }
    idx += ret;
    while (idx < _length) {
        node_t *file = file_spec_create();
        if (file == NULL) {
            break;
        }
        file_spec_path(file, (char *) _data + idx, length);
        idx += (int) length;
    }
    return idx;
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
        dir_entry_size(_entry, &value);
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
static int mms_file_dir_response(
        const unsigned char *_data,
        size_t _length) {
    if (_data == NULL || _length == 0) {
        return -1;
    }
    int idx = 0;
    if (_data[idx++] != 0xbf) {
        return idx;
    }
    if (_data[idx++] != MMS_SERVICE_FILE) {
        return idx;
    }
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return idx;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return idx;
    }
    // directory entry list flag: 0xa0
    if (_data[idx++] != 0xa0) {
        return idx;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return idx;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return idx;
    }
    unsigned int size = _data[idx++];
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return idx;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return idx;
    }
    xlist_t *list = xlist_create();
    if (list == NULL) {
        return idx;
    }
    while (idx < _length) {
        node_t *entry = dir_entry_create();
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
        return idx;
    }
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
    // domain specific flag
    if (_data[idx++] != 0xa1) {
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
    // domain name flag
    if (_data[idx++] != 0x1a) {
        return MMS_ERR_FLAG;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (_data[length + idx] != 0x1a) {
        return MMS_ERR_LENGTH;
    }
    var_spec_domain(_variable, (char *) _data + idx, length);
    idx += (int) length;
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
    var_spec_index(_variable, (char *) _data + idx, length);
    idx += (int) length;
    return idx;
}

// 解析读变量请求
static int mms_read_request(
        const unsigned char *_data,
        size_t _length) {
    if (_data == NULL || _length == 0) {
        return -1;
    }
    int idx = 0;
    if (_data[idx++] != MMS_SERVICE_READ) {
        return idx;
    }
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return idx;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return idx;
    }
    // read request service data: 0xa1
    if (_data[idx++] != 0xa1) {
        return idx;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return idx;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return idx;
    }
    // data list flag: 0xa0
    if (_data[idx++] != 0xa0) {
        return idx;
    }
    ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return idx;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return idx;
    }
    xlist_t *list = xlist_create();
    if (list == NULL) {
        return idx;
    }
    while (idx < length) {
        node_t *variable = var_spec_create();
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
    if (xlist_count(list) == 0) {
        xlist_destroy(list);
        list = NULL;
    }
    return idx;
}

static int mms_data_value(
        const unsigned char *_data,
        xvalue_t *_value) {
    if (_data == NULL || _value == NULL) {
        return -1;
    }
    int idx = 0;
    switch (_data[idx++]) {
        case 0x82: {
            idx = MMS_ERR_DATATYPE;
            break;
        }
        case 0x83: { // boolean
            if (_data[idx++] != 0x01) {
                idx = MMS_ERR_LENGTH;
                break;
            }
            xvalue_set_bool(_value, _data[idx++]);
            break;
        }
        case 0x84: { // bit string
            break;
        }
        case 0x85: { // integer
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
        case 0x86: { // unsigned integer
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
        case 0x87: { // float point
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
        case 0x89: { // octet string
            break;
        }
        case 0x8a: { // visible string
            break;
        }
        case 0x8c: { // binary time
            break;
        }
        case 0x91: { // utc time
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
    if (_data[idx] == 0x80) { // error code
        idx++;
        if (_data[idx++] != 0x01) {
            return MMS_ERR_LENGTH;
        }
        value.value._int = _data[idx++];
        acsret_value(_acsret, &value);
        return idx;
    }
    // data value
    idx = mms_data_value(_data + idx, &value);
    if (idx <= 0) {
        return idx;
    }
    acsret_value(_acsret, &value);
    return idx;
}

// 解析读服务响应
static int mms_read_response(
        const unsigned char *_data,
        size_t _length) {
    if (_data == NULL || _length == 0) {
        return -1;
    }
    int idx = 0;
    if (_data[idx++] != MMS_SERVICE_READ) {
        return MMS_ERR_FLAG;
    }
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (length != (_length - idx)) {
        return MMS_ERR_LENGTH;
    }
    // read service response data flag
    if (_data[idx++] != 0xa1) {
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
    xlist_t *list = xlist_create();
    if (list == NULL) {
        return idx;
    }
    while (idx < _length) {
        node_t *result = acsret_create();
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
    if (xlist_count(list) == 0) {
        xlist_destroy(list);
        list = NULL;
    }
    return idx;
}

static int mms_write_request(
        const unsigned char *_data,
        size_t _length) {
    if (_data == NULL || _length == 0) {
        return MMS_ERR_NULL;
    }
    int idx = 0;
    if (_data[idx++] != MMS_SERVICE_WRITE) {
        return MMS_ERR_FLAG;
    }
    unsigned int length = 0;
    int ret = mms_parse_length(_data + idx, &length);
    if (ret <= 0) {
        return MMS_ERR_LENGTH;
    }
    idx += ret;
    if (length >= (_length - idx) ||
        _data[idx + length] != 0xa0) {
        return MMS_ERR_LENGTH;
    }

    return idx;
}

static int mms_write_response(
        const unsigned char *_data,
        size_t _length) {
    if (_data == NULL || _length == 0) {
        return MMS_ERR_NULL;
    }
    int idx = 0;
    if (_data[idx++] != MMS_SERVICE_WRITE) {
        return MMS_ERR_FLAG;
    }
    return idx;
}

int mms_parse(const unsigned char *_data, size_t _length) {
    if (_data == NULL || _length == 0) {
        return -1;
    }
    int idx = 0;
    while (idx < _length) {
        if (_data[idx] == MMS_DIR_UP ||
            _data[idx] == MMS_DIR_DOWN) {
            break;
        }
        idx++;
    }
    if (idx == _length) {
        // log: not mms packet
        return -1;
    }
    if (_data[idx] == MMS_DIR_UP) {
        int ret = 0;
        idx++;
        unsigned int length = 0;
        ret = mms_parse_length(_data + idx, &length);
        if (ret <= 0) {
            return idx;
        }
        idx += ret;
        if (length != (_length - idx)) {
            return idx;
        }
        unsigned int invoke = 0;
        ret = mms_parse_invoke(_data + idx, &invoke);
        if (ret <= 0) {
            return idx;
        }
        idx += ret;
        switch (_data[idx]) {
            case 0xbf: {
                if (_data[idx + 1] == MMS_SERVICE_FILE) {
                    mms_file_dir_request(_data + idx, _length - idx);
                }
                break;
            }
            case MMS_SERVICE_READ: {
                mms_read_request(_data + idx, _length - idx);
                break;
            }
            case MMS_SERVICE_WRITE: {
                mms_write_request(_data + idx, _length - idx);
                break;
            }
            default: {
                break;
            }
        }
    } else {
        // if (_data[idx] == MMS_DIR_DOWN)
        int ret = 0;
        idx++;
        unsigned int length = 0;
        ret = mms_parse_length(_data + idx, &length);
        if (ret <= 0) {
            return idx;
        }
        idx += ret;
        if (length != (_length - idx)) {
            return idx;
        }
        unsigned int invoke_id = 0;
        ret = mms_parse_invoke(_data + idx, &invoke_id);
        if (ret <= 0) {
            return idx;
        }
        idx += ret;
        switch (_data[idx]) {
            case 0xbf: {
                if (_data[idx + 1] == MMS_SERVICE_FILE) {
                    mms_file_dir_response(_data + idx, _length - idx);
                }
                break;
            }
            case MMS_SERVICE_READ: {
                mms_read_response(_data + idx, _length - idx);
                break;
            }
            case MMS_SERVICE_WRITE: {
                mms_write_response(_data + idx, _length - idx);
                break;
            }
            default: {
                break;
            }
        }
    }
    return idx;
}


