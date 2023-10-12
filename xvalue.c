
#include "xvalue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

const char *mmsstr_set_data_auto(
        mmsstr_t *_str, const char *_data) {
    if (_str == NULL || _data == NULL) {
        return NULL;
    }
    unsigned int length = strlen(_data);
    return mmsstr_set_data(_str, _data, length);
}

const char *mmsstr_set_data(
        mmsstr_t *_str, const char *_data,
        unsigned int _length) {
    if (_str == NULL || _data == NULL ||
        _data[0] == 0 || _length == 0) {
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
        _str->data.long_str = dest;
    }
    _str->length = _length;
    return dest;
}

const char *mmsstr_data(mmsstr_t *_str) {
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
            if (_value->value._string.length >=
                STRING_SHORT_SIZE) {
                free(_value->value._string.data.long_str);
                _value->value._string.data.long_str = NULL;
            }
            _value->type = VALUE_TYPE_INVALID;
            break;
        }
        case VALUE_TYPE_STRUCT: {
            unsigned int idx = 0;
            unsigned int cnt = _value->value._struct.length;
            xvalue_t *vals = _value->value._struct._value;
            while (idx < cnt) {
                xvalue_clear(vals + idx);
                idx++;
            }
            free(vals);
            _value->type = VALUE_TYPE_INVALID;
            break;
        }
        default: {
            _value->type = VALUE_TYPE_INVALID;
            break;
        }
    }
    return 0;
}

int xvalue_to_string(const xvalue_t *_value, char *_dest) {
    if (_value == NULL || _dest == NULL) {
        return -1;
    }
    int length = 0;
    return length;
}

int xvalue_set_bool(xvalue_t *_value, unsigned char _val) {
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

int xvalue_set_float(xvalue_t *_value, float _val) {
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

int xvalue_set_int(xvalue_t *_value, int _val) {
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

int xvalue_set_uint(xvalue_t *_value, unsigned int _val) {
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
