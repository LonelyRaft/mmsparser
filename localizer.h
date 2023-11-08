//
// Created by sparrow on 2023/10/31.
//

#ifndef MMSPARSER_LOCALIZER_H
#define MMSPARSER_LOCALIZER_H

#define LANG_EN_US (1)
#define LANG_EN_UK (2)
#define LANG_ZH_CN (3)
#define LANG_ZH_TW (4)

void gen_local(int _lang);

// this interface will not return NULL/nullptr
const char *xtrans(const char *_source);

#endif //MMSPARSER_LOCALIZER_H
