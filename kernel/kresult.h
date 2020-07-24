/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _KRESULT_H_
#define _KRESULT_H_

typedef enum {
    KRESULT_OK,
    KRESULT_NO_SPACE,
    KRESULT_NOT_FOUND,
    KRESULT_INVALID_ARGUMENT,
    KRESULT_RESOURCE_SHORTAGE,
    KRESULT_OPERATION_NOT_SUPPORTED,
    KRESULT_UNIMPLEMENTED,
} kresult_t;

#endif // _KRESULT_H_
