#include "parser.h"

#include <stdio.h>

#include <vector.h>

bool parseToOPs(Tokens tokens) {

    for (int i = 0; i < tokens.cnt; i++) {
        Token t = VEC_GET(tokens, i);
        printf("[%3d] Type: %-20s Lit: %.*s\n", i, tokenTypeToStr(t.type), t.lit.len, t.lit.str);
    }

    return true;
}
