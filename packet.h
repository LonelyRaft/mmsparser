
#ifndef PACKET_MMS_H
#define PACKET_MMS_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "xlist.h"
#include "xvalue.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*********************************node*********************************/

#define NODE_TYPE_INVALID (0)
#define NODE_TYPE_FILESPEC (1)
#define NODE_TYPE_DIRENTRY (2)
#define NODE_TYPE_VARSPEC (3)
#define NODE_TYPE_UDATA (4)
#define NODE_TYPE_NAMEREQ (5)
#define NODE_TYPE_IDSTR (6)
#define NODE_TYPE_WRITRESP (7)
#define NODE_TYPE_WRITREQ (8)
#define NODE_TYPE_FOPENREQ (9)
#define NODE_TYPE_FOPENRESP (10)
#define NODE_TYPE_FREAD (11)
#define NODE_TYPE_FCLOSE (12)
#define NODE_TYPE_FREADRESP (13)
#define NODE_TYPE_INIT (14)
#define NODE_TYPE_TYPE (15)

node_t *node_create(int _type);

extern int node_destroy(node_t *_node);

extern int node_tostring(
        node_t *_node, char *_dest,
        size_t _size);

/*********************************file_spec_t*********************************/

int file_spec_path(
        node_t *_node, const char *_path,
        unsigned int _length);

/*********************************dir_entry_t*********************************/

int dir_entry_path(
        node_t *_node, const char *_path,
        unsigned int _length);

int dir_entry_size(
        node_t *_node, unsigned int _size);

int dir_entry_stamp(
        node_t *_node, const char *_stamp);

/*********************************fopen_req_t*********************************/

int fopen_req_path(
        node_t *_node, const char *_path,
        unsigned int _length);

int fopen_req_position(
        node_t *_node, unsigned int _pos);

/*********************************fopen_resp_t*********************************/

int fopen_resp_frsm(node_t *_node, unsigned int _frsm);

int fopen_resp_attr(
        node_t *_node, unsigned int _fsize,
        const char *_stamp);

/*********************************fread_t*********************************/

int fread_value(node_t *_node, unsigned int _value);

/*********************************fread_resp_t*********************************/

int fread_resp_size(node_t *_node, unsigned int _size);

int fread_resp_flag(
        node_t *_node, const unsigned char *_flag,
        unsigned char _length, unsigned char _head);

int fread_resp_follow(
        node_t *_node, unsigned char _follow);

/*********************************fclose_t*********************************/

int fclose_value(
        node_t *_node, unsigned char _updown,
        unsigned int _value);

/*********************************var_spec_t*********************************/

int var_spec_domain(
        node_t *_node, const char *_domain,
        unsigned int _length);

int var_spec_index(
        node_t *_node, const char *_index,
        unsigned int _length);

/*********************************udata_t*********************************/

const xvalue_t *udata_value(
        node_t *_node, const xvalue_t *_value);

/*********************************name_req_t*********************************/

int name_req_type(node_t *_node, int _type);

int name_req_domain(
        node_t *_node, const char *_data,
        unsigned int _length);

int name_req_next(
        node_t *_node, const char *_data,
        unsigned int _length);

/*********************************idstr_t*********************************/

int idstr_name(
        node_t *_node, const char *_data,
        unsigned int _length);

/*********************************writ_resp_t*********************************/

int writ_resp_code(
        node_t *_node, unsigned char _okay,
        unsigned char _code);

/*********************************writ_req_t*********************************/

int writ_req_value(
        node_t *_node,
        const xvalue_t *_value);

/*********************************init_t*********************************/

int init_detail_called(
        node_t *_node,
        unsigned int _value);

int init_max_calling(
        node_t *_node,
        unsigned int _calling);

int init_max_called(
        node_t *_node,
        unsigned int _called);

int init_struct_nest(
        node_t *_node,
        unsigned int _nest);

int init_version(
        node_t *_node,
        unsigned int _version);

int init_param_cbb(
        node_t *_node,
        const unsigned char *_data);

int init_services(
        node_t *_node,
        const unsigned char *_data);

int type_name(
        node_t *_node, const char *_name,
        unsigned int _length);

int type_code(node_t *_node, int _code);

int type_constraint(
        node_t *_node,
        const xvalue_t *_value);

xlist_t *type_get_constraint(
        node_t *_node);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !PACKET_MMS_H
