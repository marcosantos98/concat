#include "tokenizer.h"

#include <ctype.h>
#include <stdio.h>
#include <vector.h>

#define LOC(path, col, row) \
    (Loc) { path, col, row }

char *tokenTypeToStr(TokenType op) {
    switch (op) {
    case TT_PLUS:
        return "TT_PLUS";
    case TT_MINUS:
        return "TT_MINUS";
    case TT_MULT:
        return "TT_MULT";
    case TT_DIV:
        return "TT_DIV";
    case TT_MOD:
        return "TT_MOD";
    case TT_LT:
        return "TT_LT";
    case TT_GT:
        return "TT_GT";
    case TT_EQ:
        return "TT_EQ";
    case TT_LIT_NUMBER:
        return "TT_LIT_NUMBER";
    case TT_LIT_STR:
        return "TT_LIT_STR";
    case TT_DUMP:
        return "TT_DUMP";
    case TT_BDUMP:
        return "TT_BDUMP";
    case TT_DUP:
        return "TT_DUP";
    case TT_2DUP:
        return "TT_2DUP";
    case TT_DROP:
        return "TT_DROP";
    case TT_SWAP:
        return "TT_SWAP";
    case TT_STASH:
        return "TT_STASH";
    case TT_POP:
        return "TT_POP";
    case TT_WORD:
        return "TT_WORD";
    default:
        return "Unknown type";
    }
}

static Token makeToken(TokenType type, UnownedStr lit, Loc loc) {
    return (Token){
        .type = type,
        .lit = lit,
        .loc = loc,
    };
}

UnownedStr parseLitStr(char *const code, size_t *cursor) {
    size_t start = *cursor + 1;
    size_t end = start;
    while (code[end] != '"') {
        end++;
    }

    UnownedStr str = {
        .str = code + start,
        .len = end - start,
    };

    end++; // close "

    *cursor = end;

    return str;
}

UnownedStr parseLiteralNumber(char *const code, size_t *cursor) {

    size_t start = *cursor;
    size_t end = start;

    while (isdigit(code[end]))
        end++;

    *cursor = end;

    return (UnownedStr){
        .str = code + start,
        .len = end - start,
    };
}

UnownedStr parseIdent(char *const code, size_t *cursor) {

    size_t start = *cursor;
    size_t end = start;

    while (isalpha(code[end]))
        end++;

    *cursor = end;

    return (UnownedStr){
        .str = code + start,
        .len = end - start,
    };
}

size_t parseComment(const char *code, size_t cursor) {
    size_t end = cursor;
    while (code[end] != '\n')
        end++;
    return end;
}

UnownedStr sized(char *const code, size_t cursor, size_t size) {
    return (UnownedStr){
        .str = code + cursor,
        .len = size,
    };
}

bool tokenize(const char *path, char *const code, size_t len, Tokens *tokens) {
    size_t cursor = 0;

    int col = 1;
    int row = 1;

    while (cursor < len) {
        switch (code[cursor]) {
        case '"': {
            UnownedStr s = parseLitStr(code, &cursor);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_LIT_STR, s, l);
            VEC_ADD(tokens, t);
            col += s.len;
        } break;
        case '+': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_PLUS, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case '-': {
            if (code[cursor + 1] == '>') {
                UnownedStr s = sized(code, cursor, 2);
                Loc l = LOC(path, col, row);
                Token t = makeToken(TT_POP, s, l);
                VEC_ADD(tokens, t);
                cursor += 2;
                col += 2;
            } else {
                UnownedStr s = sized(code, cursor, 1);
                Loc l = LOC(path, col, row);
                Token t = makeToken(TT_MINUS, s, l);
                VEC_ADD(tokens, t);
                cursor++;
                col++;
            }
        } break;
        case '*': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_MULT, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case '/': {
            if (code[cursor + 1] == '/') {
                size_t old = cursor;
                cursor = parseComment(code, cursor);
                col += cursor - old;
            } else {
                UnownedStr s = sized(code, cursor, 1);
                Loc l = LOC(path, col, row);
                Token t = makeToken(TT_DIV, s, l);
                VEC_ADD(tokens, t);
                cursor++;
                col++;
            }
        } break;
        case '%': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_MOD, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case '<': {
            if (code[cursor + 1] == '-') {
                UnownedStr s = sized(code, cursor, 2);
                Loc l = LOC(path, col, row);
                Token t = makeToken(TT_STASH, s, l);
                VEC_ADD(tokens, t);
                cursor += 2;
                col += 2;
            } else {
                UnownedStr s = sized(code, cursor, 1);
                Loc l = LOC(path, col, row);
                Token t = makeToken(TT_LT, s, l);
                VEC_ADD(tokens, t);
                cursor++;
                col++;
            }
        } break;
        case '>': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_GT, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case '=': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_EQ, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case '?': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_DUMP, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case '!': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_BDUMP, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case '.': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_DUP, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case ':': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_2DUP, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case ',': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_DROP, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case ';': {
            UnownedStr s = sized(code, cursor, 1);
            Loc l = LOC(path, col, row);
            Token t = makeToken(TT_SWAP, s, l);
            VEC_ADD(tokens, t);
            cursor++;
            col++;
        } break;
        case '\t':
        case ' ': {
            cursor++;
            col++;
        } break;
        case '\n':
        case '\r': {
            cursor++;
            col = 1;
            row += 1;
        } break;
        default: {
            if (isdigit(code[cursor])) {
                UnownedStr s = parseLiteralNumber(code, &cursor);
                Loc l = LOC(path, col, row);
                Token t = makeToken(TT_LIT_NUMBER, s, l);
                VEC_ADD(tokens, t);
                col += s.len;
            } else if (isalpha(code[cursor])) {
                UnownedStr s = parseIdent(code, &cursor);
                Loc l = LOC(path, col, row);
                Token t = makeToken(TT_WORD, s, l);
                VEC_ADD(tokens, t);
                col += s.len;
            } else {
                printf("Char not handled %c\n", code[cursor]);
                exit(1);
            }
        }
        }
    }

    return true;
}
