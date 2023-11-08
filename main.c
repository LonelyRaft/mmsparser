#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int main(int argc, char *argv[]) {
    unsigned char *buffer =
            (unsigned char *) malloc(1024 * 56);
    if (buffer == NULL) {
        return -1;
    }
    FILE *data = fopen("../message.txt", "rb");
    if (data == NULL) {
        return -2;
    }
    int length;
    do {
        length = fscanf(data, " %[^\n]", buffer);
        if (length > 0 && buffer[0] == '#') {
            printf("%s\n", (char *) buffer);
            continue;
        }
        if (length > 0) {
            length = (int) strlen((char *) buffer);
            length = make_msg(buffer, length);
            service_t *service = mms_parse(buffer, length);
            length = mms_tostring(
                    service, (char *) buffer, 10240);
            if (length > 0) {
                printf("%s\n", buffer);
            }
            mms_destroy(service);
        }
    } while (length >= 0);
    fclose(data);
    return 0;
}
