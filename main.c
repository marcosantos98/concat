#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <bench.h>
#include <io.h>
#include <strb.h>
#include <vector.h>

#define DEBUG 0

#define MEASURE(bPtr, strmsg)            \
    do {                                 \
        if (DEBUG)                       \
            BENCH_MEASURE(bPtr, strmsg); \
    } while (0)

#define _ (void)

// StartTokenizer
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
    TT_WORD,
    TT_DUMP,
    TT_BDUMP,
    TT_DUP,
    TT_2DUP,
    TT_DROP,
    TT_SWAP,
    TT_STASH,
    TT_POP,
} TokenType;

typedef struct {
    const char *path;
    int col;
    int row;
} Loc;

#define LOC(path, col, row) \
    (Loc) { path, col, row }

void printloc(Loc l) {
    printf("%s:%d:%d:", l.path, l.row, l.col);
}

typedef struct {
    char *str;
    int len;
} UStr;

#define US_CMP(ustr, cstr) strncmp(ustr.str, cstr, strlen(cstr))

UStr makeUStr(char *const data, int off, int len) {
    return (UStr){data + off, len};
}

typedef struct {
    TokenType t;
    Loc l;
    UStr lit;
} Token;

typedef struct {
    Token *data;
    int cnt;
    int cap;
} Tokens;

Token makeToken(TokenType t, UStr lit, Loc l) {
    return (Token){t, l, lit};
}

UStr parseWord(char *const code, size_t *cursor) {
    size_t start = *cursor;
    size_t end = start;

    while (isalpha(code[end]))
        end++;

    *cursor = end;

    return (UStr){code + start, end - start};
}

UStr parseStrLiteral(char *const code, size_t *cursor) {
    size_t start = *cursor;
    size_t end = ++start;

    while (code[end] != '"')
        end++;

    UStr s = (UStr){code + start, end - start};

    *cursor = end + 1;
    return s;
}

UStr parseNumberLiteral(char *const code, size_t *cursor) {
    size_t start = *cursor;
    size_t end = start;

    while (isdigit(code[end]))
        end++;

    *cursor = end;
    return (UStr){code + start, end - start};
}

size_t parseComment(const char *code, size_t cursor) {
    size_t end = cursor;
    while (code[end] != 10)
        end++;
    return end;
}

bool tokenize(char *const code, size_t len, const char *path, Tokens *tokens) {
    size_t cursor = 0;

    int col = 1;
    int row = 1;

    while (cursor < len) {
        switch (code[cursor]) {
        case '"': {
            UStr s = parseStrLiteral(code, &cursor);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_LIT_STR, s, l));
            col += s.len;
        } break;
        case '+': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_PLUS, s, l));
            cursor++;
            col++;
        } break;
        case '-': {
            if (code[cursor + 1] == '>') {
                UStr s = makeUStr(code, cursor, 2);
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, makeToken(TT_POP, s, l));
                cursor += 2;
                col += 2;
            } else {
                UStr s = makeUStr(code, cursor, 1);
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, makeToken(TT_MINUS, s, l));
                cursor++;
                col++;
            }
        } break;
        case '*': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_MULT, s, l));
            cursor++;
            col++;
        } break;
        case '/': {
            if (code[cursor + 1] == '/') {
                size_t old = cursor;
                cursor = parseComment(code, cursor);
                col += cursor - old;
            } else {
                UStr s = makeUStr(code, cursor, 1);
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, makeToken(TT_DIV, s, l));
                cursor++;
                col++;
            }
        } break;
        case '%': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_MOD, s, l));
            cursor++;
            col++;
        } break;
        case '<': {
            if (code[cursor + 1] == '-') {
                UStr s = makeUStr(code, cursor, 2);
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, makeToken(TT_STASH, s, l));
                cursor += 2;
                col += 2;
            } else {
                UStr s = makeUStr(code, cursor, 1);
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, makeToken(TT_LT, s, l));
                cursor++;
                col++;
            }
        } break;
        case '>': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_GT, s, l));
            cursor++;
            col++;
        } break;
        case '=': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_EQ, s, l));
            cursor++;
            col++;
        } break;
        case '?': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_DUMP, s, l));
            cursor++;
            col++;
        } break;
        case '!': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_BDUMP, s, l));
            cursor++;
            col++;
        } break;
        case '.': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_DUP, s, l));
            cursor++;
            col++;
        } break;
        case ':': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_2DUP, s, l));
            cursor++;
            col++;
        } break;
        case ',': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_DROP, s, l));
            cursor++;
            col++;
        } break;
        case ';': {
            UStr s = makeUStr(code, cursor, 1);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, makeToken(TT_SWAP, s, l));
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
                UStr s = parseNumberLiteral(code, &cursor);
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, makeToken(TT_LIT_NUMBER, s, l));
                col += s.len;
            } else if (isalpha(code[cursor])) {
                UStr s = parseWord(code, &cursor);
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, makeToken(TT_WORD, s, l));
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
// EndTokenizer

// StartParser
typedef enum {
    OP_NOP = 0,
    OP_BINOP,
    OP_LIT_NUMBER,
    OP_LIT_STR,
    OP_WORD,
    OP_DUMP,
    OP_BDUMP,
    OP_DUP,
    OP_2DUP,
    OP_DROP,
    OP_SWAP,
    OP_STASH,
    OP_POP,
} OpType;

typedef enum {
    BT_PLUS,
    BT_MINUS,
    BT_MULT,
    BT_DIV,
    BT_MOD,
    BT_LT,
    BT_GT,
    BT_EQ,
} BinopType;

typedef enum {
    W_DEFINED = 0,
    W_PUTD,
    W_LOOP,
    W_END,
    W_DO,
    W_PUTC,
    W_PRINTLN,
    W_PRINT,
    W_IF,
    W_ELSE,
    W_ENDIF,
} WordType;

typedef struct {
    Loc l;
    OpType t;
    int op;
    int link;
} Op;

char *opTypeToStr(Op op) {
    switch (op.t) {
    case OP_NOP:
        return "OP_NOP";
    case OP_BINOP: {
        switch (op.op) {
        case BT_PLUS:
            return "OP_PLUS";
        case BT_MINUS:
            return "OP_MINUS";
        case BT_MULT:
            return "OP_MULT";
        case BT_DIV:
            return "OP_DIV";
        case BT_MOD:
            return "OP_MOD";
        case BT_LT:
            return "OP_LT";
        case BT_GT:
            return "OP_GT";
        case BT_EQ:
            return "OP_EQ";
        }
    } break;
    case OP_LIT_NUMBER:
        return "OP_LIT_NUMBER";
    case OP_LIT_STR:
        return "OP_LIT_STR";
    case OP_WORD: {
        switch (op.op) {
        case W_PUTD:
            return "W_PUTD";
        case W_LOOP:
            return "W_LOOP";
        case W_END:
            return "W_END";
        case W_DO:
            return "W_DO";
        case W_PUTC:
            return "W_PUTC";
        case W_PRINTLN:
            return "W_PRINTLN";
        case W_PRINT:
            return "W_PRINT";
        case W_IF:
            return "W_IF";
        case W_ELSE:
            return "W_ELSE";
        case W_ENDIF:
            return "W_ENDIF";
        default:
            return "W_DEFINED";
        }
    } break;
    case OP_DUMP:
        return "OP_DUMP";
    case OP_BDUMP:
        return "OP_BDUMP";
    case OP_DUP:
        return "OP_DUP";
    case OP_2DUP:
        return "OP_2DUP";
    case OP_DROP:
        return "OP_DROP";
    case OP_SWAP:
        return "OP_SWAP";
    case OP_STASH:
        return "OP_STASH";
    case OP_POP:
        return "OP_POP";
    default:
        return "Unknown type";
    }
    return "";
}

typedef struct {
    UStr *data;
    int cnt;
    int cap;
} UStrList;

typedef struct {
    Op *data;
    int cnt;
    int cap;
    UStrList strTable;
} Program;

Op parseBinopToken(Token t) {
    switch (t.t) {
    case TT_PLUS:
        return (Op){.l = t.l, .t = OP_BINOP, .op = BT_PLUS, .link = 0};
    case TT_MINUS:
        return (Op){.l = t.l, .t = OP_BINOP, .op = BT_MINUS, .link = 0};
    case TT_MULT:
        return (Op){.l = t.l, .t = OP_BINOP, .op = BT_MULT, .link = 0};
    case TT_DIV:
        return (Op){.l = t.l, .t = OP_BINOP, .op = BT_DIV, .link = 0};
    case TT_MOD:
        return (Op){.l = t.l, .t = OP_BINOP, .op = BT_MOD, .link = 0};
    case TT_LT:
        return (Op){.l = t.l, .t = OP_BINOP, .op = BT_LT, .link = 0};
    case TT_GT:
        return (Op){.l = t.l, .t = OP_BINOP, .op = BT_GT, .link = 0};
    case TT_EQ:
        return (Op){.l = t.l, .t = OP_BINOP, .op = BT_EQ, .link = 0};
    default:
        return (Op){t.l, OP_NOP, 0, 0};
    }
    return (Op){t.l, OP_NOP, 0, 0};
}

Op parseWordToken(Token t) {
    Op o = (Op){.l = t.l, .t = OP_WORD, .op = W_DEFINED, .link = 0};

    if (US_CMP(t.lit, "sout") == 0)
        o.op = W_PUTD;
    else if (US_CMP(t.lit, "loop") == 0)
        o.op = W_LOOP;
    else if (US_CMP(t.lit, "endif") == 0)
        o.op = W_ENDIF;
    else if (US_CMP(t.lit, "do") == 0)
        o.op = W_DO;
    else if (US_CMP(t.lit, "putc") == 0)
        o.op = W_PUTC;
    else if (US_CMP(t.lit, "println") == 0)
        o.op = W_PRINTLN;
    else if (US_CMP(t.lit, "print") == 0)
        o.op = W_PRINT;
    else if (US_CMP(t.lit, "if") == 0)
        o.op = W_IF;
    else if (US_CMP(t.lit, "else") == 0)
        o.op = W_ELSE;
    else if (US_CMP(t.lit, "end") == 0)
        o.op = W_END;

    return o;
}

bool parse(Tokens tokens, Program *prog) {
    int i = 0;

    while (i < tokens.cnt) {
        Token t = VEC_GET(tokens, i);
        switch (t.t) {
        case TT_PLUS:
        case TT_MINUS:
        case TT_MULT:
        case TT_DIV:
        case TT_MOD:
        case TT_LT:
        case TT_GT:
        case TT_EQ: {
            VEC_ADD(prog, parseBinopToken(t));
        } break;
        case TT_LIT_NUMBER: {
            Op o = (Op){.l = t.l, .t = OP_LIT_NUMBER, .op = atoi(t.lit.str), .link = 0};
            VEC_ADD(prog, o);
        } break;
        case TT_LIT_STR: {
            Op o = (Op){.l = t.l, .t = OP_LIT_STR, .op = prog->strTable.cnt, .link = 0};
            VEC_ADD(prog, o);
            VEC_ADD(&prog->strTable, t.lit);
        } break;
        case TT_WORD: {
            VEC_ADD(prog, parseWordToken(t));
        } break;
        case TT_DUMP: {
            Op o = (Op){.l = t.l, .t = OP_DUMP, .op = 0, .link = 0};
            VEC_ADD(prog, o);
        } break;
        case TT_BDUMP: {
            Op o = (Op){.l = t.l, .t = OP_BDUMP, .op = 0, .link = 0};
            VEC_ADD(prog, o);
        } break;
        case TT_DUP: {
            Op o = (Op){.l = t.l, .t = OP_DUP, .op = 0, .link = 0};
            VEC_ADD(prog, o);
        } break;
        case TT_2DUP: {
            Op o = (Op){.l = t.l, .t = OP_2DUP, .op = 0, .link = 0};
            VEC_ADD(prog, o);
        } break;
        case TT_DROP: {
            Op o = (Op){.l = t.l, .t = OP_DROP, .op = 0, .link = 0};
            VEC_ADD(prog, o);
        } break;
        case TT_SWAP: {
            Op o = (Op){.l = t.l, .t = OP_SWAP, .op = 0, .link = 0};
            VEC_ADD(prog, o);
        } break;
        case TT_STASH: {
            Op o = (Op){.l = t.l, .t = OP_STASH, .op = 0, .link = 0};
            VEC_ADD(prog, o);
        } break;
        case TT_POP: {
            Op o = (Op){.l = t.l, .t = OP_POP, .op = 0, .link = 0};
            VEC_ADD(prog, o);
        } break;
        }
        i++;
    }

    Loc l = VEC_GET(tokens, i - 1).l;
    l.col = 1;
    l.row++;
    Op o = (Op){.l = l, .t = OP_NOP, .op = 0, .link = 0};
    VEC_ADD(prog, o);
    return false;
}
// EndParser

#define MAX_STACK 100

#include <stdarg.h>

typedef enum {
    ERR_OVERFLOW,
    ERR_UNDERFLOW,
    ERR_UNCLOSED_LOOP,
    ERR_UNCLOSED_IF,
    ERR_NO_DO,
    ERR_NO_STR,
} ErrorType;

void error(ErrorType type, Op op, const char *msg, ...) {

    va_list list;
    va_start(list, msg);

    printf("E: ");
    printloc(op.l);
    printf(" ");

    switch (type) {
    case ERR_OVERFLOW:
        printf("Stack overflow! Reach limit of %d. ", MAX_STACK);
        break;
    case ERR_UNDERFLOW:
        printf("Stack underflow. ");
        break;
    default:
        break;
    }

    vprintf(msg, list);

    va_end(list);

    // FIXME: This leaks all the memory not fread. Let it leek?
    exit(1);
}

// StartLinker

bool isWord(Op o, WordType t) {
    return o.t == OP_WORD && (WordType)o.op == t;
}

void controlFlowLink(Program *prog) {
    int ip = 0;

    while (prog->data[ip].t != OP_NOP) {
        Op op = prog->data[ip];
        if (op.t != OP_WORD) {
            ip++;
            continue;
        }

        if (isWord(op, W_LOOP)) {
            int loopIp = ip;
            int doIp = -1;
            int end = ip + 1;

            int loopCnt = 1;
            while (loopCnt > 0) {
                Op endOP = prog->data[end];
                if (endOP.t == OP_NOP)
                    error(ERR_UNCLOSED_LOOP, endOP, "`do` requires an `end` keyword.\n");

                if (isWord(endOP, W_DO) && doIp == -1)
                    doIp = end;

                if (isWord(endOP, W_LOOP))
                    loopCnt++;
                if (isWord(endOP, W_END)) {
                    loopCnt--;
                    if (loopCnt == 0)
                        break;
                }
                end++;
            }

            if (doIp == -1)
                error(ERR_NO_DO, prog->data[loopIp], "`do` keyword not found.\n");

            prog->data[doIp].link = end + 1;
            prog->data[end].link = loopIp;

        } else if (isWord(op, W_IF)) {
            int ifIp = ip;
            int end = ip + 1;
            int elseIp = -1;

            int ifCnt = 1;
            while (ifCnt > 0) {
                Op endOP = prog->data[end];
                if (endOP.t == OP_NOP)
                    error(ERR_UNCLOSED_IF, endOP, "`if` requires and `endif` keyword\n");

                if (isWord(endOP, W_IF))
                    ifCnt++;
                if (isWord(endOP, W_ELSE) && ifCnt == 1)
                    elseIp = end;
                if (isWord(endOP, W_ENDIF)) {
                    ifCnt--;
                    if (ifCnt == 0)
                        break;
                }
                end++;
            }

            if (elseIp != -1) {
                prog->data[ifIp].link = elseIp + 1;
                prog->data[elseIp].link = end + 1;
            } else {
                prog->data[ifIp].link = end + 1;
            }
        }
        ip++;
    }
}

// EndLinker

// StartInterpeter

char *opToSyntax(Op op) {
    switch (op.t) {
    case OP_BINOP: {
        switch (op.op) {
        case BT_PLUS:
            return "+";
        case BT_MINUS:
            return "-";
        case BT_MULT:
            return "*";
        case BT_DIV:
            return "/";
        case BT_MOD:
            return "%";
        case BT_LT:
            return "<";
        case BT_GT:
            return ">";
        case BT_EQ:
            return "=";
        }
    } break;
    case OP_WORD: {
        switch (op.op) {
        case W_PUTD:
            return "putd";
        case W_LOOP:
            return "loop";
        case W_END:
            return "end";
        case W_DO:
            return "do";
        case W_PUTC:
            return "putc";
        case W_PRINTLN:
            return "println";
        case W_PRINT:
            return "print";
        case W_IF:
            return "if";
        case W_ELSE:
            return "else";
        case W_ENDIF:
            return "endif";
        }
    } break;
    case OP_DUMP:
        return "?";
    case OP_BDUMP:
        return "!";
    case OP_DUP:
        return ".";
    case OP_2DUP:
        return ":";
    case OP_DROP:
        return ",";
    case OP_SWAP:
        return ";";
    case OP_STASH:
        return "<-";
    case OP_POP:
        return "->";
    default:
        return "Unknown type";
    }
    return "";
}

void canPopAmt(int sp, int amt, Op o) {
    if (sp - amt < 0)
        error(ERR_UNDERFLOW, o, "`%s` requires at least %d value(s) on the stack.\n", opToSyntax(o), amt);
}

int pop(int *stack, int *sp) {
    return stack[--(*sp)];
}

void tryPush(int *stack, int *sp, int operand, Op o) {
    if (*sp + 1 >= MAX_STACK)
        error(ERR_OVERFLOW, o, "Can't push literal number %d\n", operand);
    stack[(*sp)++] = operand;
}

void interpetBinop(int *stack, int *sp, Op o) {
    canPopAmt(*sp, 2, o);
    int top = pop(stack, sp);
    int ut = pop(stack, sp);
    switch (o.op) {
    case BT_PLUS:
        tryPush(stack, sp, top + ut, o);
        break;
    case BT_MINUS:
        tryPush(stack, sp, top - ut, o);
        break;
    case BT_MULT:
        tryPush(stack, sp, top * ut, o);
        break;
    case BT_DIV:
        tryPush(stack, sp, top / ut, o);
        break;
    case BT_MOD:
        tryPush(stack, sp, top % ut, o);
        break;
    case BT_LT:
        tryPush(stack, sp, top < ut, o);
        break;
    case BT_GT:
        tryPush(stack, sp, top > ut, o);
        break;
    case BT_EQ:
        tryPush(stack, sp, top == ut, o);
        break;
    }
}

void interpetWord(int *stack, int *sp, int *ip, Op o, Program prog) {
    switch (o.op) {
    case W_DEFINED: {
        *ip += 1;
    } break;
    case W_PUTD: {
        canPopAmt(*sp, 1, o);
        int top = pop(stack, sp);
        printf("%d", top);
        *ip += 1;
    } break;
    case W_LOOP: {
        *ip += 1;
    } break;
    case W_END: {
        *ip = o.link;
    } break;
    case W_DO: {
        canPopAmt(*sp, 1, o);
        int top = pop(stack, sp);
        if (top)
            *ip += 1;
        else
            *ip = o.link;
    } break;
    case W_PUTC: {
        canPopAmt(*sp, 1, o);
        int top = pop(stack, sp);
        printf("%c", top);
        *ip += 1;
    } break;
    case W_PRINTLN: {
        canPopAmt(*sp, 1, o);
        int top = pop(stack, sp);
        if (prog.strTable.cnt == 0)
            error(ERR_NO_STR, o, "`println` or `print` requires an string index. Be sure to push a string before using it.\n");
        printf("%.*s\n", VEC_GET(prog.strTable, top).len, VEC_GET(prog.strTable, top).str);
        *ip += 1;
    } break;
    case W_PRINT: {
        canPopAmt(*sp, 1, o);
        int top = pop(stack, sp);
        if (prog.strTable.cnt == 0)
            error(ERR_NO_STR, o, "`println` or `print` requires an string index. Be sure to push a string before using it.\n");
        printf("%.*s", prog.strTable.data[top].len, prog.strTable.data[top].str);
        *ip += 1;

    } break;
    case W_IF: {
        canPopAmt(*sp, 1, o);
        int top = pop(stack, sp);
        if (top)
            *ip += 1;
        else
            *ip = o.link;
    } break;
    case W_ELSE: {
        *ip = o.link;
    } break;
    case W_ENDIF: {
        *ip += 1;
    } break;
    }
}

bool interpet(Program prog) {
    int stack[MAX_STACK] = {0};
    int sp = 0;
    int backStack[MAX_STACK] = {0};
    int bsp = 0;

    int ip = 0;
    while (VEC_GET(prog, ip).t != OP_NOP) {
        Op o = VEC_GET(prog, ip);
        switch (o.t) {
        case OP_BINOP: {
            interpetBinop(stack, &sp, o);
            ip++;
        } break;
        case OP_LIT_NUMBER:
        case OP_LIT_STR: {
            tryPush(stack, &sp, o.op, o);
            ip++;
        } break;
        case OP_WORD: {
            interpetWord(stack, &sp, &ip, o, prog);
        } break;
        case OP_DUMP: {
            printf("> Stack Dump:\n");
            for (int j = 0; j < sp; j++) {
                printf("[%d] %d\n", j, stack[j]);
            }
            printf("< End Stack Dump.\n");
            ip++;
        } break;
        case OP_BDUMP: {
            printf("> Back Stack Dump:\n");
            for (int j = 0; j < sp; j++) {
                printf("[%d] %d\n", j, backStack[j]);
            }
            printf("< End Back Stack Dump.\n");
            ip++;
        } break;
        case OP_DUP: {
            canPopAmt(sp, 1, o);
            tryPush(stack, &sp, stack[sp - 1], o);
            ip++;
        } break;
        case OP_2DUP: {
            canPopAmt(sp, 2, o);
            int top = stack[sp - 1];
            int ut = stack[sp - 2];
            tryPush(stack, &sp, ut, o);
            tryPush(stack, &sp, top, o);
            ip++;
        } break;
        case OP_DROP: {
            canPopAmt(sp, 1, o);
            pop(stack, &sp);
            ip++;
        } break;
        case OP_SWAP: {
            canPopAmt(sp, 2, o);
            int top = pop(stack, &sp);
            int ut = pop(stack, &sp);
            tryPush(stack, &sp, top, o);
            tryPush(stack, &sp, ut, o);
            ip++;
        } break;
        case OP_STASH: {
            canPopAmt(sp, 1, o);
            int top = pop(stack, &sp);
            if (sp - top < 0)
                error(ERR_UNDERFLOW, o, "Trying to stash %d values on the stack, but only %d are available.\n", top, sp);
            if (bsp + top >= MAX_STACK)
                error(ERR_OVERFLOW, o, "Can't stash %d values on back stack.\n", top);

            for (int n = 0; n < top; n++)
                backStack[bsp++] = stack[--sp];
            ip++;
        } break;
        case OP_POP: {
            canPopAmt(sp, 1, o);
            int top = pop(stack, &sp);
            if (bsp - top < 0)
                error(ERR_UNDERFLOW, o, "Trying to pop %d values from the back stack, but only %d are available.\n", top, sp);
            if (sp + top >= MAX_STACK)
                error(ERR_OVERFLOW, o, "Can't pop %d values on stack.\n", top);

            for (int n = 0; n < top; n++)
                stack[sp++] = backStack[--bsp];
            ip++;
        } break;
        default: {
        } break;
        }
    }

    if (sp != 0) {
        // TODO: Move this to error() ?
        printf("E: Unhandled data on the stack.\n");
        for (int i = sp - 1; i >= 0; i--) {
            printf("[%d] %d\n", i, stack[i]);
        }
    }

    return true;
}

void printOps(Program prog) {
    if (DEBUG) {
        for (int i = 0; i < prog.cnt; i++) {
            printf("[%d] OP: %s\n", i, opTypeToStr(prog.data[i]));
            printf("    > operand: %d\n", prog.data[i].op);
            printf("    > link: %d\n", prog.data[i].link);
            printf("    > loc: ");
            printloc(prog.data[i].l);
            printf("\n");
        }
    }
}

int main(int argc, char **argv) {

    if (argc < 2)
        return 1;

    _ *argv++;

    const char *path = *argv++;

    bool gen = false;
    if (argc == 3) {
        const char *genArg = *argv++;
        if (strncmp(genArg, "gen", 3) == 0)
            gen = true;
    }

    if (gen && system("qbe -h 1> /dev/null") != 0) {
        printf("QBE backend not installed system wide.\n");
        return 1;
    }

    if (gen && system("clang --version 1> /dev/null") != 0) {
        printf("Clang not installed system wide.\n");
        return 1;
    }

    size_t len;
    char *code = read_file(path, &len);
    if (code == NULL)
        return 1;

    bench b = {0};
    BENCH_START(&b);
    Tokens tokens = {0};
    tokenize(code, len, path, &tokens);
    MEASURE(&b, "Tokenize");

    Program prog = {0};
    parse(tokens, &prog);
    MEASURE(&b, "Parse tokens");

    printOps(prog);

    BENCH_START(&b);
    controlFlowLink(&prog);
    MEASURE(&b, "ControlFlowLink");

    BENCH_START(&b);
    interpet(prog);
    MEASURE(&b, "Interpet");

    VEC_FREE(prog.strTable);
    VEC_FREE(prog);
    VEC_FREE(tokens);

    free(code);

    return 0;
}
