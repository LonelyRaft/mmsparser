
#include "xvalue.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "node.h"

const char *mmsstr_set_data_auto(mmsstr_t *_str, const char *_data) {
    if (_str == NULL || _data == NULL) {
        return NULL;
    }
    unsigned int length = strlen(_data);
    return mmsstr_set_data(_str, _data, length);
}

const char *mmsstr_set_data(mmsstr_t *_str, const char *_data,
                            unsigned int _length) {
    if (_str == NULL || _data == NULL || _length == 0) {
        return NULL;
    }
    char *dest = _str->data.short_str;
    if (_length >= STRING_SHORT_SIZE) {
        dest = (char *) malloc(_length + 1);
        if (dest == NULL) {
            return NULL;
        }
    }
    memcpy(dest, _data, _length);
    dest[_length] = 0;
    if (_str->length >= STRING_SHORT_SIZE) {
        free(_str->data.long_str);
        _str->data.long_str = NULL;
    }
    if (_length >= STRING_SHORT_SIZE) {
        _str->data.long_str = dest;
    }
    _str->length = _length;
    return dest;
}

const char *mmsstr_data(const mmsstr_t *_str) {
    if (_str == NULL) {
        return NULL;
    }
    if (_str->length >= STRING_SHORT_SIZE) {
        return _str->data.long_str;
    }
    return _str->data.short_str;
}

void mmsstr_clear(mmsstr_t *_str) {
    if (_str == NULL) {
        return;
    }
    if (_str->length >= STRING_SHORT_SIZE) {
        free(_str->data.long_str);
        _str->data.long_str = NULL;
    }
    _str->length = 0;
}

int xvalue_clear(xvalue_t *_value) {
    if (_value == NULL) {
        return -1;
    }
    switch (_value->type) {
        case VALUE_TYPE_STRING: {
            mmsstr_clear(&_value->value._string);
            break;
        }
        case VALUE_TYPE_STRUCT: {
            xlist_destroy(_value->value._struct);
            break;
        }
        default: {
            break;
        }
    }
    memset(_value, 0, sizeof(xvalue_t));
    _value->type = VALUE_TYPE_INVALID;
    return 0;
}

int xvalue_to_string(
        const xvalue_t *_value,
        char *_dest, size_t _size) {
    if (_value == NULL || _dest == NULL) {
        return -1;
    }
    int length = 0;
    switch (_value->type) {
        case VALUE_TYPE_STRUCT: {
            length = snprintf(_dest, _size - 1, "structure:{");
            xlist_t *list = _value->value._struct;
            node_t *node = xlist_begin(list);
            while (node && length < (_size - 1)) {
                _dest[length++] = ' ';
                int ret = node_tostring(
                        node, _dest + length,
                        _size - length);
                if (ret < 0) {
                    break;
                }
                length += ret;
                node = xlist_next(list);
            }
            if (length < (_size - 1)) {
                _dest[length++] = '}';
            }
            if (length >= (_size - 1)) {
                length = (int) _size - 1;
            }
            _dest[length] = 0;
            break;
        }
        case VALUE_TYPE_BOOL: {
            if (_value->value._bool) {
                length = snprintf(_dest, _size - 1, "boolean:{true}");
                break;
            }
            length = snprintf(_dest, _size - 1, "boolean:{false}");
            break;
        }
        case VALUE_TYPE_BITS: {
            unsigned int size = _value->value._string.length;
            unsigned char *data = (unsigned char *) mmsstr_data(
                    &_value->value._string);
            if (size == 0) {
                break;
            }
            size--;
            size <<= 3; // *8
            size -= data[0];
            const char *header = "bit-string:{length:%u, data:";
            length = snprintf(_dest, _size - 1, header, size);
            if (length >= (_size - 1)) {
                break;
            }
            int idx = 0;
            while (idx < size && length < (_size - 1)) {
                int byte_idx = (idx >> 3) + 1;
                int bit_idx = (idx & 0x07);
                unsigned char bitval = data[byte_idx] & (0x80 >> bit_idx);
                if (bitval) {
                    _dest[length++] = '1';
                } else {
                    _dest[length++] = '0';
                }
                idx++;
            }
            if (length < (_size - 1)) {
                _dest[length++] = '}';
            }
            _dest[length] = 0;
            break;
        }
        case VALUE_TYPE_INT: {
            length = snprintf(
                    _dest, _size - 1, "integer:{%d}",
                    _value->value._int);
            break;
        }
        case VALUE_TYPE_UINT: {
            length = snprintf(
                    _dest, _size - 1, "unsigned integer:{%u}",
                    _value->value._uint);
            break;
        }
        case VALUE_TYPE_FLOAT: {
            length = snprintf(
                    _dest, _size - 1, "float:{%f}",
                    _value->value._float);
            break;
        }
        case VALUE_TYPE_OCTSTR: {
            unsigned int octlen = (_value->value._string.length * 8 + 2) / 3;
            length = snprintf(_dest, _size - 1, "octet-string:{length:%u, data:", octlen);
            if (length >= (_size - 1)) {
                length = (int) _size - 1;
                break;
            }
            // octet string data
            const unsigned char *octstr =
                    (unsigned char *) mmsstr_data(
                            &_value->value._string);
            // bit length of the octet string
            int bitlen = (int) _value->value._string.length;
            bitlen <<= 3;
            int idx = 0; // bit idx of the octtet string
            int adder = 0;
            if (bitlen % 3 == 1) {
                adder = 2;
            } else if (bitlen % 3 == 2) {
                adder = 1;
            }
            unsigned char value = 0;
            while (idx < bitlen) {
                int byte_idx = (idx >> 3); // byte index of the octet string
                int bit_idx = (idx & 0x07); // bit index of the bytes
                unsigned char flag = octstr[byte_idx] & (0x80 >> bit_idx);
                value <<= 1;
                if (flag) {
                    value += 1;
                }
                idx++;
                if ((idx + adder) % 3 == 0) {
                    value += '0';
                    if (length < (_size - 1)) {
                        _dest[length++] = (char) value;
                        value = 0;
                        continue;
                    }
                    break;
                }
            }
            if (length < (_size - 1)) {
                _dest[length++] = '}';
            }
            _dest[length] = 0;
            break;
        }
        case VALUE_TYPE_STRING: {
            length = snprintf(
                    _dest, _size - 1, "string:{length:%u, data:%s}",
                    _value->value._string.length,
                    mmsstr_data(&_value->value._string));
            break;
        }
        case VALUE_TYPE_BINTIME: {
            time_t secs =
                    _value->value._btime.days * 86400 +
                    5113 * 86400 + // 1984 - 1970 = 5113
                    _value->value._btime.msecs / 1000;
            unsigned int msecs =
                    _value->value._btime.msecs % 1000;
            struct tm curr = {0};
            gmtime_s(&curr, &secs);
            curr.tm_year += 1900;
            curr.tm_mon += 1;
            const char *fmt = "binary-time:{UTC:%04d-%02d-%02d %02d:%02d:%02d.%03d}";
            length = snprintf(_dest, _size - 1, fmt,
                              curr.tm_year, curr.tm_mon, curr.tm_mday,
                              curr.tm_hour, curr.tm_min, curr.tm_sec, msecs);
            break;
        }
        case VALUE_TYPE_UTCTIME: {
            struct tm curr = {0};
            time_t value = _value->value._utc.seconds;
            gmtime_s(&curr, &value);
            curr.tm_year += 1900;
            curr.tm_mon += 1;
            unsigned int msecs =
                    (unsigned int) (_value->value._utc.real * 1000);
            const char *fmt = "UTC-time:{%04d-%02d-%02d %02d:%02d:%02d.%03d}";
            length = snprintf(_dest, _size - 1, fmt,
                              curr.tm_year, curr.tm_mon, curr.tm_mday,
                              curr.tm_hour, curr.tm_min, curr.tm_sec, msecs);
            break;
        }
        default: {
            break;
        }
    }
    return length;
}

int xvalue_set_bool(
        xvalue_t *_value, unsigned char _val) {
    if (_value == NULL) {
        return -1;
    }
    if (_value->type == VALUE_TYPE_INVALID) {
        _value->type = VALUE_TYPE_BOOL;
    }
    if (_value->type != VALUE_TYPE_BOOL) {
        return -2;
    }
    _value->value._bool = _val;
    return 0;
}

int xvalue_set_float(
        xvalue_t *_value, float _val) {
    if (_value == NULL) {
        return -1;
    }
    if (_value->type == VALUE_TYPE_INVALID) {
        _value->type = VALUE_TYPE_FLOAT;
    }
    if (_value->type != VALUE_TYPE_FLOAT) {
        return -2;
    }
    _value->value._float = _val;
    return 0;
}

int xvalue_set_int(
        xvalue_t *_value, int _val) {
    if (_value == NULL) {
        return -1;
    }
    if (_value->type == VALUE_TYPE_INVALID) {
        _value->type = VALUE_TYPE_INT;
    }
    if (_value->type != VALUE_TYPE_INT) {
        return -2;
    }
    _value->value._int = _val;
    return 0;
}

int xvalue_set_uint(
        xvalue_t *_value, unsigned int _val) {
    if (_value == NULL) {
        return -1;
    }
    if (_value->type == VALUE_TYPE_INVALID) {
        _value->type = VALUE_TYPE_UINT;
    }
    if (_value->type != VALUE_TYPE_UINT) {
        return -2;
    }
    _value->value._uint = _val;
    return 0;
}

int xvalue_set_string(
        xvalue_t *_value, const char *_val,
        unsigned int _length) {
    if (_value == NULL) {
        return -1;
    }
    if (_value->type == VALUE_TYPE_INVALID) {
        _value->type = VALUE_TYPE_STRING;
    }
    if (_value->type != VALUE_TYPE_STRING) {
        return -2;
    }
    mmsstr_set_data(&_value->value._string, _val, _length);
    return 0;
}

int xvalue_set_bitstr(
        xvalue_t *_value, const char *_val,
        unsigned int _length) {
    if (_value == NULL) {
        return -1;
    }
    if (_value->type == VALUE_TYPE_INVALID) {
        _value->type = VALUE_TYPE_BITS;
    }
    if (_value->type != VALUE_TYPE_BITS) {
        return -2;
    }
    mmsstr_set_data(&_value->value._string, _val, _length);
    return 0;
}

int xvalue_set_octstr(
        xvalue_t *_value, const char *_val,
        unsigned int _length) {
    if (_value == NULL) {
        return -1;
    }
    if (_value->type == VALUE_TYPE_INVALID) {
        _value->type = VALUE_TYPE_OCTSTR;
    }
    if (_value->type != VALUE_TYPE_OCTSTR) {
        return -2;
    }
    mmsstr_set_data(&_value->value._string, _val, _length);
    return 0;
}

int xvalue_set_bintime(
        xvalue_t *_value,
        const unsigned char *_bin_time) {
    if (_value == NULL) {
        return -1;
    }
    if (_value->type == VALUE_TYPE_INVALID) {
        _value->type = VALUE_TYPE_BINTIME;
    }
    if (_value->type != VALUE_TYPE_BINTIME) {
        return -2;
    }
    int idx = 0;
    unsigned int msecs = 0;
    while (idx < 4) {
        msecs <<= 8;
        msecs += _bin_time[idx];
        idx++;
    }
    unsigned int days = _bin_time[idx++];
    days <<= 8;
    days += _bin_time[idx];
    _value->value._btime.days = days;
    _value->value._btime.msecs = msecs;
    return 0;
}

int xvalue_set_utctime(
        xvalue_t *_value,
        const unsigned char *_utc_time) {
    if (_value == NULL) {
        return -1;
    }
    if (_value->type == VALUE_TYPE_INVALID) {
        _value->type = VALUE_TYPE_UTCTIME;
    }
    if (_value->type != VALUE_TYPE_UTCTIME) {
        return -2;
    }
    int idx = 0;
    unsigned int secs = 0;
    while (idx < 4) {
        secs <<= 8;
        secs += _utc_time[idx];
        idx++;
    }
    _value->value._utc.seconds = secs;
    idx = 0;
    float value = 1.0f;
    float real = 0;
    while (idx < 16) {
        value /= 2;
        int byte_idx = ((idx >> 3) + 4);
        int bit_idx = (idx & 0x07);
        unsigned char flag = _utc_time[byte_idx];
        flag &= (0x80 >> bit_idx);
        if (flag) {
            real += value;
        }
        idx++;
    }
    _value->value._utc.real = real;
    return 0;
}

int xvalue_set_struct(
        xvalue_t *_value, xlist_t *_struct) {
    if (_value == NULL || _struct == NULL) {
        return -1;
    }
    if (_value->type == VALUE_TYPE_INVALID) {
        _value->type = VALUE_TYPE_STRUCT;
    }
    if (_value->type != VALUE_TYPE_STRUCT) {
        return -2;
    }
    _value->value._struct = _struct;
    return 0;
}
