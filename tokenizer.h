#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include "str.h"

#include <stdbool.h>
#include <stddef.h>

#define LIT_MAX_SIZE 64

typedef struct {
    const char *path;
    int col;
    int row;
} Loc;

typedef enum {
    TT_PLUS,
    TT_MINUS,
    TT_MULT,
    TT_DIV,
    TT_MOD,
    TT_LT,
    TT_GT,
    TT_EQ,
    TT_LIT_NUMBER,
    TT_LIT_STR,
    TT_DUMP,
    TT_BDUMP,
    TT_DUP,
    TT_2DUP,
    TT_DROP,
    TT_SWAP,
    TT_STASH,
    TT_POP,
    TT_WORD,
} TokenType;

char *tokenTypeToStr(TokenType);

typedef struct {
    UnownedStr lit;
    Loc loc;
    TokenType type;
} Token;

typedef struct {
    Token *data;
    int cap;
    int cnt;
} Tokens;

bool tokenize(const char *, char *const code, size_t len, Tokens *tokens);

#endif //_TOKENIZER_H_
