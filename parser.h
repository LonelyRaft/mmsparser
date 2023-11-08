
#ifndef MMS_PARSER_H
#define MMS_PARSER_H

#include "packet.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct service_t service_t;

const char *error_tostring(int _error);

service_t *mms_parse(const unsigned char *_data, size_t _length);

int mms_tostring(const service_t *_serice, char *_dest, size_t _size);

int mms_destroy(service_t *_service);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !MMS_PARSER_H
