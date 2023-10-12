
#ifndef MMS_PARSER_H
#define MMS_PARSER_H

#include "packet.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct resp_t {
    unsigned int invoke;
    unsigned int type;
    xlist_t *list;
} resp_t;

typedef struct request_t
{
    unsigned int invoke;
    unsigned int type;
    xlist_t *list;
}request_t;

int mms_parse(const unsigned char *_data, size_t _length);


#ifdef __cplusplus
}
#endif // __cplusplus


#endif // !MMS_PARSER_H

