#ifndef XVALUE_H
#define XVALUE_H

#include "xlist.h"

#ifdef __cplusplus
extern "C" {
#endif  // !__cplusplus

#define STRING_SHORT_SIZE (32)
typedef struct mmsstr_t {
    unsigned int length;
    union data {
        char short_str[STRING_SHORT_SIZE];
        char *long_str;
    } data;
} mmsstr_t;

const char *mmsstr_set_data_auto(
        mmsstr_t *_str, const char *_data);

const char *mmsstr_set_data(
        mmsstr_t *_str, const char *_data,
        unsigned int _length);

const char *mmsstr_data(const mmsstr_t *_str);

void mmsstr_clear(mmsstr_t *_str);

typedef struct utc_time_t{
    unsigned int seconds;
    float real;
}utc_time_t;

typedef struct binary_time_t
{
    unsigned int days;
    unsigned int msecs;
}binary_time_t;

#define VALUE_TYPE_INVALID (0)
#define VALUE_TYPE_ERROR (0x80)    // error code
#define VALUE_TYPE_STRUCT (0xa2)   // Struct
#define VALUE_TYPE_BOOL (0x83)     // Boolean
#define VALUE_TYPE_BITS (0x84)     // Bit String
#define VALUE_TYPE_INT (0x85)      // Integer
#define VALUE_TYPE_UINT (0x86)     // Unsigned
#define VALUE_TYPE_FLOAT (0x87)    // Float Point
#define VALUE_TYPE_OCTSTR (0x89)   // Octet String
#define VALUE_TYPE_STRING (0x8a)   // Visiable String
#define VALUE_TYPE_BINTIME (0x8c)  // Binary Time
#define VALUE_TYPE_UTCTIME (0x91)  // UTC Time

typedef struct xvalue_t xvalue_t;

typedef struct xvalue_t {
    int type;
    union val_data {
        unsigned char _bool;
        int _int;
        unsigned int _uint;
        binary_time_t _btime;
        utc_time_t _utc;
        float _float;
        mmsstr_t _string;
        xlist_t *_struct;
    } value;
} xvalue_t;

int xvalue_clear(xvalue_t *_value);

int xvalue_to_string(
        const xvalue_t *_value,
        char *_dest, size_t _size);

int xvalue_set_bool(
        xvalue_t *_value,
        unsigned char _val);

int xvalue_set_float(xvalue_t *_value, float _val);

int xvalue_set_int(xvalue_t *_value, int _val);

int xvalue_set_uint(
        xvalue_t *_value,
        unsigned int _val);

int xvalue_set_string(
        xvalue_t *_value,
        const char *_val, unsigned int _length);

int xvalue_set_bitstr(
        xvalue_t *_value,
        const char *_val, unsigned int _length);

int xvalue_set_octstr(
        xvalue_t *_value,
        const char *_val, unsigned int _length);

int xvalue_set_bintime(
        xvalue_t *_value,
        const unsigned char* _bin_time);

int xvalue_set_utctime(
        xvalue_t *_value,
        const unsigned char* _utc_time);

int xvalue_set_struct(
        xvalue_t *_value,
        xlist_t *_struct);

#ifdef __cplusplus
}
#endif  // !__cplusplus

#endif  // !XVALUE_H
