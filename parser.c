#include "parser.h"

#include <stdio.h>

#include <vector.h>

Op handleWord(Tokens tokens, int tokenIdx) {
    Token t = VEC_GET(tokens, tokenIdx);
    WordType t = W_DEFINED;
    if (US_CMP(t.lit, "println") == 0) {
        t = W_PRINTLN;
    }
    return (Op){t, t};
}

bool parseToOPs(Tokens tokens, Prog *prog) {

    int i = 0;
    while (tokens.cnt > i) {

        Token t = VEC_GET(tokens, i);

        switch (t.type) {
        case TT_LIT_NUMBER: {
            Op o = (Op){t, atoi(t.lit.str)};
            VEC_ADD(prog, o);
        } break;
        case TT_LIT_STR: {
            Op o = (Op){t, prog->strTable.cnt};
            VEC_ADD(&prog->strTable, t.lit);
            VEC_ADD(prog, o);
        } break;
        case TT_DUP:
        case TT_2DUP:
        case TT_SWAP:
        case TT_DROP:
        case TT_BDUMP:
        case TT_DUMP:
        case TT_POP:
        case TT_STASH:
        case TT_MINUS:
        case TT_MULT:
        case TT_MOD:
        case TT_DIV:
        case TT_LT:
        case TT_GT:
        case TT_EQ:
        case TT_PLUS: {
            Op o = (Op){t, 0};
            VEC_ADD(prog, o);
        } break;
        case TT_WORD: {
            VEC_ADD(prog, handleWord(tokens, i));
        } break;
        }
        i++;
    }

    for (int i = 0; i < prog->cnt; i++) {
        Op o = prog->data[i];
        printf("%s\n", tokenTypeToStr(o.t.type));
    }

    return true;
}
