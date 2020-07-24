/* 
 * Copyright (c) 2015 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#include <ctype.h>

int isprint(int c) {
    return (c > 0x1f) && (c < 0x7f);
}
