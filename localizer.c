//
// Created by sparrow on 2023/10/31.
//

#include <stdlib.h>
#include <string.h>
#include "localizer.h"
#include "node.h"
#include "xlist.h"

/*****************************str_pair_t*****************************/

typedef struct str_pair_t {
    node_t parent;
    size_t length;
    const char *source;
    const char *target;
} str_pair_t;

static int pair_destroy(node_t *_node) {
    free(_node);
    return 0;
}

static node_t *pair_create(
        const char *_source,
        const char *_target) {
    static const node_op_t nodeop = {
            pair_destroy,
            NULL,
    };
    if (_source == NULL || _target == NULL) {
        return NULL;
    }
    str_pair_t *pair = (str_pair_t *) malloc(sizeof(str_pair_t));
    memset(pair, 0, sizeof(str_pair_t));
    pair->parent.op = &nodeop;
    pair->length = strlen(_source);
    pair->source = _source;
    pair->target = _target;
    return (node_t *) pair;
}

/*****************************trans_t*****************************/

// locale language configuration
typedef struct trans_t {
    int lang;
    xlist_t *pairs;
} trans_t;
trans_t g_locale = {0, NULL};

// translate core
static const char *translate(
        trans_t *_trans, const char *_source) {
    const char *target = "";
    if (_source == NULL) {
        return target;
    }
    target = _source;
    if (_trans == NULL) {
        return target;
    }
    size_t length = strlen(_source);
    str_pair_t *pair = (str_pair_t *) xlist_begin(_trans->pairs);
    while (pair != NULL) {
        if (pair->length == length &&
            0 == strcmp(pair->source, _source)) {
            pair = (str_pair_t *) xlist_next(_trans->pairs);
            continue;
        }
        target = pair->target;
        break;
    }
    return target;
}

// create translate string pair
static int append_pair(
        trans_t *_trans,
        const char *_source,
        const char *_target) {
    if (_trans == NULL || _trans->pairs == NULL ||
        _source == NULL || _target == NULL) {
        return -1;
    }
    node_t *pair = pair_create(_source, _target);
    if (pair == NULL) {
        return -2;
    }
    return xlist_append(_trans->pairs, pair);
}

static void gen_chinese_cn(trans_t *_trans) {
    if (_trans == NULL) {
        return;
    }
    append_pair(_trans, "message parsing error:{error:%s, position:%u}", "报文解析错误:{错误:%s, 位置:%u}");
    append_pair(_trans, "fileAttr:{size:%u, UTC_stamp:%04d-%02d-%02d %02d:%02d:%02d}",
                "文件属性:{大小:%u, UTC时间戳:%04d-%02d-%02d %02d:%02d:%02d}");
    append_pair(_trans, "fileOpenRequest:{path:%s, position:%u}", "文件打开请求:{路径:%s, 位置:%u}");
    append_pair(_trans, "fileOpenResponse:{fileHandle:%u, ", "文件打开响应:{文件句柄:%u, ");
    append_pair(_trans, "fileReadRequest:{fileHandle:%u}", "文件读取请求:{文件句柄:%u}");
    append_pair(_trans, "fileReadResponse:{size:%u", "文件读取响应:{大小：%u");
    append_pair(_trans, ", start:0x%02x 0x%02x 0x%02x 0x%02x", ", 起始字节:0x%02x 0x%02x 0x%02x 0x%02x");
    append_pair(_trans, ", end:0x%02x 0x%02x 0x%02x 0x%02x}", ", 终止字节:0x%02X 0x%02x 0x%02x 0x%02x}");
    append_pair(_trans, ", follow:%c}", ", 后续:%c}");
    append_pair(_trans, "fileCloseRequest:{fileHandle:%u}", "文件关闭请求:{文件句柄:%u}");
    append_pair(_trans, "fileCloseResponse:{success}", "文件关闭响应:{成功}");
    append_pair(_trans, "fileCloseResponse:{failed}", "文件关闭响应:{失败}");
    append_pair(_trans, "fileDirRequest:{", "文件目录请求:{");
    append_pair(_trans, "pathSpec:{path:%s}", "指定路径:{路径:%s}");
    append_pair(_trans, "", "");
    append_pair(_trans, "directoryEntry:{path:%s, ", "目录项:{路径:%s, ");
    append_pair(_trans, "", "");
    append_pair(_trans, "varSpec:{%s/%s}", "指定变量:{%s/%s}");
    append_pair(_trans, "", "");
    append_pair(_trans, "", "");
    append_pair(_trans, "", "");
    append_pair(_trans, "", "");
    append_pair(_trans, "", "");
    append_pair(_trans, "", "");
    append_pair(_trans, "", "");
}

static void gen_chinese_tw(trans_t *_trans) {

}

static void gen_english_us(trans_t *_trans) {

}

static void gen_english_uk(trans_t *_trans) {

}

typedef struct gen_lang_t {
    int lang;

    void (*gen_action)(trans_t *);
} gen_lang_t;

// set locale interface
void gen_local(int _lang) {
    static const gen_lang_t g_langs[] = {
            {LANG_ZH_CN, gen_chinese_cn},
            {LANG_ZH_TW, gen_chinese_tw},
            {LANG_EN_US, gen_english_us},
            {LANG_EN_UK, gen_english_uk},
            {0, NULL},
    };
    if (_lang == g_locale.lang) {
        return;
    }
    const gen_lang_t *gen_lang = g_langs;
    while (gen_lang->gen_action) {
        if (gen_lang->lang == _lang) {
            break;
        }
        gen_lang++;
    }
    if (gen_lang->gen_action != NULL) {
        xlist_destroy(g_locale.pairs);
        if (g_locale.pairs == NULL) {
            g_locale.pairs = xlist_create();
        }
        gen_lang->gen_action(&g_locale);
    }
}

// translate interface
const char *xtrans(const char *_source) {
    return translate(&g_locale, _source);
}
