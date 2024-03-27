#ifndef _STRING_H_
#define _STRING_H_

#include <string.h>

typedef struct {
    char *str;
    int len;
} UnownedStr;

#define US_CMP(us, cstr) strncmp(us.str, cstr, us.len)

#endif //_STRING_H_
