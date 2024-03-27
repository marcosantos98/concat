#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdbool.h>

#include "str.h"
#include "tokenizer.h"

typedef struct {
    Token t;
    int op;
} Op;

typedef struct {
    UnownedStr *data;
    int cnt;
    int cap;
} StringTable;

typedef struct {
    Op *data;
    int cnt;
    int cap;
    StringTable strTable;
} Prog;

bool parseToOPs(Tokens, Prog *);

#endif //_PARSER_H_
