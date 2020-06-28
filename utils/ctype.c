#include <ctype.h>

int isprint(int c) {
    return (c > 0x1f) && (c < 0x7f);
}
