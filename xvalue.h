#ifndef XVALUE_H
#define XVALUE_H

#ifdef __cplusplus
extern "C" {
#endif // !__cplusplus

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

const char *mmsstr_data(mmsstr_t *_str);

void mmsstr_clear(mmsstr_t *_str);

#define VALUE_TYPE_INVALID (0)
#define VALUE_TYPE_STRUCT (2) // Struct
#define VALUE_TYPE_BOOL (3) // Boolean
#define VALUE_TYPE_BITS (4) // Bit String
#define VALUE_TYPE_INT (5) // Integer
#define VALUE_TYPE_UINT (6) // Unsigned
#define VALUE_TYPE_FLOAT (7) // Float Point
#define VALUE_TYPE_OCTSTR (9) // Octet String
#define VALUE_TYPE_STRING (10) // Visiable String
#define VALUE_TYPE_TIME (12) // Binary Time
#define VALUE_TYPE_UTC (17) // UTC Time

typedef struct xvalue_t xvalue_t;

typedef struct xstruct_t {
    unsigned int length;
    xvalue_t *_value;
} xstruct_t;

typedef struct xvalue_t {
    int type;
    union val_data {
        unsigned char _bool;
        char _char;
        unsigned char _uchar;
        short _short;
        unsigned short _ushort;
        int _int;
        unsigned int _uint;
        long long _llong;
        unsigned long long _ullong;
        float _float;
        double _double;
        mmsstr_t _string;
        xstruct_t _struct;
    } value;
} xvalue_t;

int xvalue_clear(xvalue_t *_value);

int xvalue_to_string(const xvalue_t *_value, char *_dest);

int xvalue_set_bool(xvalue_t *_value, unsigned char _val);

int xvalue_set_float(xvalue_t *_value, float _val);

int xvalue_set_int(xvalue_t *_value, int _val);

int xvalue_set_uint(xvalue_t *_value, unsigned int _val);

#ifdef __cplusplus
}
#endif // !__cplusplus

#endif // !XVALUE_H
