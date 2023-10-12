#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

static int make_msg(unsigned char *buffer, int length) {
    if (buffer == NULL || length <= 0) {
        return 0;
    }
    int writ = 0;
    int read = 0;
    while (read < length) {
        unsigned char tmp_val = buffer[read];
        if ('0' <= tmp_val && '9' >= tmp_val) {
            tmp_val = tmp_val - '0';
        } else if ('a' <= tmp_val && 'f' >= tmp_val) {
            tmp_val = tmp_val - 'a' + 10;
        } else if ('A' <= tmp_val && 'F' >= tmp_val) {
            tmp_val = tmp_val - 'A' + 10;
        } else {
            break;
        }
        if (read & 1) {
            buffer[writ] += tmp_val;
            writ++;
        } else {
            tmp_val <<= 4;
            buffer[writ] = tmp_val;
        }
        read++;
    }
    if (read != length) {
        return -1;
    }
    return writ;
}

static int read_msg(const char *_path, unsigned char *buffer, int size) {
    if (_path == NULL || buffer == NULL) {
        return 0;
    }
    FILE *data = fopen(_path, "rb");
    if (data == NULL) {
        return -1;
    }
    int length = (int) fread(buffer, 1, size, data);
    fclose(data);
    if (length > 0) {
        length = make_msg(buffer, length);
    }
    return length;
}


int main(int argc, char *argv[]) {
    unsigned char *buffer = (unsigned char *) malloc(10240);
    if (buffer == NULL) {
        return -1;
    }
    int length = read_msg("../message.txt", buffer, 10240);
    if (length > 0) {
        mms_parse(buffer, length);
    }
    return 0;
}
