#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "tokenizer.h"

#include <bench.h>
#include <io.h>
#include <strb.h>
#include <vector.h>

#define DEBUG 1

#define MEASURE(bPtr, strmsg)            \
    do {                                 \
        if (DEBUG)                       \
            BENCH_MEASURE(bPtr, strmsg); \
    } while (0)

#define _ (void)

typedef enum {
    OPT_NOP = 0,
    OPT_PLUS,
    OPT_MINUS,
    OPT_MULT,
    OPT_DIV,
    OPT_MOD,
    OPT_LT,
    OPT_GT,
    OPT_EQ,
    OPT_LIT_NUMBER,
    OPT_LIT_STR,
    OPT_IDENT,
    OPT_DUMP,
    OPT_BDUMP,
    OPT_DUP,
    OPT_2DUP,
    OPT_DROP,
    OPT_SWAP,
    OPT_STASH,
    OPT_POP,
} OPType;

char *optypeCStr(OPType op) {
    switch (op) {
    case OPT_PLUS:
        return "OPT_PLUS";
    case OPT_MINUS:
        return "OPT_MINUS";
    case OPT_MULT:
        return "OPT_MULT";
    case OPT_DIV:
        return "OPT_DIV";
    case OPT_MOD:
        return "OPT_MOD";
    case OPT_LT:
        return "OPT_LT";
    case OPT_GT:
        return "OPT_GT";
    case OPT_EQ:
        return "OPT_EQ";
    case OPT_LIT_NUMBER:
        return "OPT_LIT_NUMBER";
    case OPT_LIT_STR:
        return "OPT_LIT_STR";
    case OPT_IDENT:
        return "OPT_IDENT";
    case OPT_DUMP:
        return "OPT_DUMP";
    case OPT_BDUMP:
        return "OPT_BDUMP";
    case OPT_DUP:
        return "OPT_DUP";
    case OPT_2DUP:
        return "OPT_2DUP";
    case OPT_DROP:
        return "OPT_DROP";
    case OPT_SWAP:
        return "OPT_SWAP";
    case OPT_STASH:
        return "OPT_STASH";
    case OPT_POP:
        return "OPT_POP";
    default:
        return "Unknown type";
    }
}

typedef struct {
    char *cstr;
    int len;
} String;

typedef struct {
    String *data;
    int cnt;
    int cap;
} Strings;

#define LOC(path, col, row) \
    (Loc) { path, col, row }

typedef struct {
    OPType type;
    int op;
    Loc loc;
} OP;

#define OP_NOP(l) \
    (OP) { .type = OPT_NOP, .loc = l }
#define OP_PLUS(l) \
    (OP) { .type = OPT_PLUS, .loc = l }
#define OP_MINUS(l) \
    (OP) { .type = OPT_MINUS, .loc = l }
#define OP_MULT(l) \
    (OP) { .type = OPT_MULT, .loc = l }
#define OP_DIV(l) \
    (OP) { .type = OPT_DIV, .loc = l }
#define OP_MOD(l) \
    (OP) { .type = OPT_MOD, .loc = l }
#define OP_LT(l) \
    (OP) { .type = OPT_LT, .loc = l }
#define OP_GT(l) \
    (OP) { .type = OPT_GT, .loc = l }
#define OP_EQ(l) \
    (OP) { .type = OPT_EQ, .loc = l }
#define OP_LIT_NUMBER(o, l) \
    (OP) { .type = OPT_LIT_NUMBER, .op = o, .loc = l }
#define OP_LIT_STR(sIdx, l) \
    (OP) { .type = OPT_LIT_STR, .op = sIdx, .loc = l }
#define OP_IDENT(o, l) \
    (OP) { .type = OPT_IDENT, .op = o, .loc = l }
#define OP_DUMP(l) \
    (OP) { .type = OPT_DUMP, .loc = l }
#define OP_BDUMP(l) \
    (OP) { .type = OPT_BDUMP, .loc = l }
#define OP_DUP(l) \
    (OP) { .type = OPT_DUP, .loc = l }
#define OP_2DUP(l) \
    (OP) { .type = OPT_2DUP, .loc = l }
#define OP_DROP(l) \
    (OP) { .type = OPT_DROP, .loc = l }
#define OP_SWAP(l) \
    (OP) { .type = OPT_SWAP, .loc = l }
#define OP_STASH(l) \
    (OP) { .type = OPT_STASH, .loc = l }
#define OP_POP(l) \
    (OP) { .type = OPT_POP, .loc = l }

#define PUTD 0
#define LOOP 1
#define END 2
#define DO 3
#define PUTC 4
#define PRINTLN 5
#define PRINT 6
#define IF 7
#define ELSE 8
#define ENDIF 9

char *identCstr(int ident) {
    switch (ident) {
    case PUTD:
        return "PUTD";
    case LOOP:
        return "LOOP";
    case END:
        return "END";
    case DO:
        return "DO";
    case PUTC:
        return "PUTC";
    case PRINTLN:
        return "PRINTLN";
    case PRINT:
        return "PRINT";
    case IF:
        return "IF";
    case ELSE:
        return "ELSE";
    case ENDIF:
        return "ENDIF";
    default:
        return "Unknown ident";
    }
}

typedef struct {
    OP *data;
    int cnt;
    int cap;
} Program;

#define MAX_STACK 100

typedef struct {
    Program program;
    int ip;
    int stack[MAX_STACK];
    int sp;
    int backStack[MAX_STACK];
    int bsp;
} VM;

static VM vm = {0};
static Strings stringTable = {0};

// void old_tokenize(const char *path, const char *code, size_t len) {
//     size_t cursor = 0;
//
//     strb tmp = strb_init(1024);
//
//     int col = 1;
//     int row = 1;
//
//     while (cursor < len) {
//         // printf("Parsing %c at %s:%d:%d\n", code[cursor], path, row, col);
//         switch (code[cursor]) {
//         case '"': {
//             String s = {0};
//             s.len = parseStr(code, &cursor, &tmp);
//             s.cstr = malloc(sizeof(char) * s.len);
//             memcpy(s.cstr, tmp.str, s.len);
//             tmp.cnt = 0;
//             VEC_ADD(&stringTable, s);
//
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_LIT_STR(stringTable.cnt - 1, l));
//
//             col += s.len;
//         } break;
//         case '+': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_PLUS(l));
//             cursor++;
//             col++;
//         } break;
//         case '-': {
//             if (code[cursor + 1] == '>') {
//                 Loc l = LOC(path, col, row);
//                 VEC_ADD(&vm.program, OP_POP(l));
//                 cursor += 2;
//                 col += 2;
//             } else {
//                 Loc l = LOC(path, col, row);
//                 VEC_ADD(&vm.program, OP_MINUS(l));
//                 cursor++;
//                 col++;
//             }
//         } break;
//         case '*': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_MULT(l));
//             cursor++;
//             col++;
//         } break;
//         case '/': {
//             if (code[cursor + 1] == '/') {
//                 size_t old = cursor;
//                 cursor = parseComment(code, cursor);
//                 col += cursor - old;
//             } else {
//
//                 Loc l = LOC(path, col, row);
//                 VEC_ADD(&vm.program, OP_DIV(l));
//                 cursor++;
//                 col++;
//             }
//         } break;
//         case '%': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_MOD(l));
//             cursor++;
//             col++;
//         } break;
//         case '<': {
//             if (code[cursor + 1] == '-') {
//                 Loc l = LOC(path, col, row);
//                 VEC_ADD(&vm.program, OP_STASH(l));
//                 cursor += 2;
//                 col += 2;
//             } else {
//                 Loc l = LOC(path, col, row);
//                 VEC_ADD(&vm.program, OP_LT(l));
//                 cursor++;
//                 col++;
//             }
//         } break;
//         case '>': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_GT(l));
//             cursor++;
//             col++;
//         } break;
//         case '=': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_EQ(l));
//             cursor++;
//             col++;
//         } break;
//         case '?': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_DUMP(l));
//             cursor++;
//             col++;
//         } break;
//         case '!': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_BDUMP(l));
//             cursor++;
//             col++;
//         } break;
//         case '.': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_DUP(l));
//             cursor++;
//             col++;
//         } break;
//         case ':': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_2DUP(l));
//             cursor++;
//             col++;
//         } break;
//         case ',': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_DROP(l));
//             cursor++;
//             col++;
//         } break;
//         case ';': {
//             Loc l = LOC(path, col, row);
//             VEC_ADD(&vm.program, OP_SWAP(l));
//             cursor++;
//             col++;
//         } break;
//         case '\t':
//         case ' ': {
//             cursor++;
//             col++;
//         } break;
//         case '\n':
//         case '\r': {
//             cursor++;
//             col = 1;
//             row += 1;
//         } break;
//         default: {
//             if (isdigit(code[cursor])) {
//                 cursor = parseLiteralNumber(code, cursor, &tmp);
//                 Loc l = LOC(path, col, row);
//                 OP op = OP_LIT_NUMBER(atoi(tmp.str), l);
//                 VEC_ADD(&vm.program, op);
//                 col += tmp.cnt - 1; // Don't include the NULL terminator
//                 tmp.cnt = 0;
//             } else if (isalpha(code[cursor])) {
//                 cursor = parseIdent(code, cursor, &tmp);
//
//                 Loc l = LOC(path, col, row);
//                 if (strncmp("sout", tmp.str, 4) == 0) {
//                     VEC_ADD(&vm.program, OP_IDENT(PUTD, l));
//                 } else if (strncmp("loop", tmp.str, 4) == 0) {
//                     VEC_ADD(&vm.program, OP_IDENT(LOOP, l));
//                 } else if (strncmp("endif", tmp.str, 5) == 0) {
//                     VEC_ADD(&vm.program, OP_IDENT(ENDIF, l));
//                 } else if (strncmp("end", tmp.str, 3) == 0) {
//                     VEC_ADD(&vm.program, OP_IDENT(END, l));
//                 } else if (strncmp("do", tmp.str, 2) == 0) {
//                     VEC_ADD(&vm.program, OP_IDENT(DO, l));
//                 } else if (strncmp("putc", tmp.str, 4) == 0) {
//                     VEC_ADD(&vm.program, OP_IDENT(PUTC, l));
//                 } else if (strncmp("println", tmp.str, 7) == 0) {
//                     VEC_ADD(&vm.program, OP_IDENT(PRINTLN, l));
//                 } else if (strncmp("print", tmp.str, 5) == 0) {
//                     VEC_ADD(&vm.program, OP_IDENT(PRINT, l));
//                 } else if (strncmp("if", tmp.str, 2) == 0) {
//                     VEC_ADD(&vm.program, OP_IDENT(IF, l));
//                 } else if (strncmp("else", tmp.str, 4) == 0) {
//                     VEC_ADD(&vm.program, OP_IDENT(ELSE, l));
//                 } else {
//                     printf("Ident not handled! %s\n", tmp.str);
//                     exit(1);
//                 }
//                 col += tmp.cnt - 1;
//                 tmp.cnt = 0;
//             } else {
//                 printf("Char not handled %c\n", code[cursor]);
//                 exit(1);
//             }
//         }
//         }
//     }
//
//     Loc l = LOC(path, col, row);
//     VEC_ADD(&vm.program, OP_NOP(l));
//     strb_free(tmp);
// }

#include <stdarg.h>

typedef enum {
    ERR_OVERFLOW,
    ERR_UNDERFLOW,
    ERR_UNCLOSED_LOOP,
    ERR_UNCLOSED_IF,
    ERR_NO_DO,
    ERR_NO_STR,
} ErrorType;

void error(ErrorType type, OP op, const char *msg, ...) {

    va_list list;
    va_start(list, msg);

    printf("E: %s:%d:%d: ", op.loc.path, op.loc.row, op.loc.col);

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

typedef struct {
    int loopIp;
    int doIp;
    int endIp;
} loop;

typedef struct {
    loop *data;
    int cnt;
    int cap;
} loopinfo;

typedef struct {
    int ifIp;
    int elseIp;
    int endIp;
} iff;

typedef struct {
    iff *data;
    int cnt;
    int cap;
} ifinfo;

static loopinfo loops = {0};
static ifinfo ifs = {0};

void controlFlowLink() {
    int ip = 0;

    while (vm.program.data[ip].type != OPT_NOP) {
        OP op = vm.program.data[ip];
        if (op.type != OPT_IDENT) {
            ip++;
            continue;
        }

        if (op.op == LOOP) {
            int loopIp = ip;
            int doIp = -1;
            int end = ip + 1;

            int loopCnt = 1;
            while (loopCnt > 0) {
                OP endOP = vm.program.data[end];
                if (endOP.type == OPT_NOP)
                    error(ERR_UNCLOSED_LOOP, endOP, "`do` requires an `end` keyword.\n");

                if (endOP.type == OPT_IDENT && endOP.op == DO && doIp == -1)
                    doIp = end;

                if (endOP.type == OPT_IDENT && endOP.op == LOOP)
                    loopCnt++;
                if (endOP.type == OPT_IDENT && endOP.op == END) {
                    loopCnt--;
                    if (loopCnt == 0)
                        break;
                }
                end++;
            }

            if (doIp == -1)
                error(ERR_NO_DO, vm.program.data[loopIp], "`do` keyword not found.\n");

            loop l = (loop){.loopIp = loopIp, .doIp = doIp, .endIp = end};
            VEC_ADD(&loops, l);
        } else if (op.op == IF) {
            int ifIp = ip;
            int end = ip + 1;
            int elseIp = -1;

            int ifCnt = 1;
            while (ifCnt > 0) {
                OP endOP = vm.program.data[end];
                if (endOP.type == OPT_NOP)
                    error(ERR_UNCLOSED_IF, endOP, "`if` requires and `endif` keyword\n");

                if (endOP.type == OPT_IDENT && endOP.op == IF)
                    ifCnt++;
                if (endOP.type == OPT_IDENT && endOP.op == ELSE && ifCnt == 1)
                    elseIp = end;
                if (endOP.type == OPT_IDENT && endOP.op == ENDIF) {
                    ifCnt--;
                    if (ifCnt == 0)
                        break;
                }
                end++;
            }

            iff i = (iff){.ifIp = ifIp, .endIp = end, .elseIp = elseIp};
            VEC_ADD(&ifs, i);
        }
        ip++;
    }
}

void interpet(VM *vm, Program program, bool gen) {

    strb data = strb_init(32);

    char snbuf[256];
    strb_append(&data, "data $putd_fmt = { b \"%d\", b 0 }\n");
    strb_append(&data, "data $putc_fmt = { b \"%c\", b 0 }\n");

    for (int i = 0; i < stringTable.cnt; i++) {
        snprintf(snbuf, 256, "data $str_%d = { b \"%.*s\", b 0 }\n", i, stringTable.data[i].len, stringTable.data[i].cstr);
        strb_append(&data, snbuf);
    }
    strb code = strb_init(32);

    strb_append(&code, "export function w $main() {\n");
    strb_append(&code, "@start\n");

    while (program.data[vm->ip].type != OPT_NOP) {
        OP op = program.data[vm->ip];
        switch (op.type) {
        case OPT_LIT_NUMBER:
        case OPT_LIT_STR: {
            if (vm->sp + 1 >= MAX_STACK)
                error(ERR_OVERFLOW, op, "Can't push literal number %d\n", op.op);

            vm->stack[vm->sp++] = op.op;
            vm->ip++;
        } break;
        case OPT_PLUS: {
            if (vm->sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "+ requires at least two values on the stack.\n");

            int top = vm->stack[--vm->sp];
            int t2 = vm->stack[--vm->sp];
            vm->stack[vm->sp++] = top + t2;
            vm->ip++;
        } break;
        case OPT_MINUS: {
            if (vm->sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "- requires at least two values on the stack.\n");

            int top = vm->stack[--vm->sp];
            int t2 = vm->stack[--vm->sp];
            vm->stack[vm->sp++] = top - t2;
            vm->ip++;
        } break;
        case OPT_MULT: {
            if (vm->sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "* requires at least two values on the stack.\n");

            int top = vm->stack[--vm->sp];
            int t2 = vm->stack[--vm->sp];
            vm->stack[vm->sp++] = top * t2;
            vm->ip++;
        } break;
        case OPT_DIV: {
            if (vm->sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "/ requires at least two values on the stack.\n");

            int top = vm->stack[--vm->sp];
            int t2 = vm->stack[--vm->sp];
            // TODO: Check for division by zero
            vm->stack[vm->sp++] = top / t2;
            vm->ip++;
        } break;
        case OPT_MOD: {
            if (vm->sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "% requires at least two values on the stack.\n");

            int top = vm->stack[--vm->sp];
            int t2 = vm->stack[--vm->sp];
            vm->stack[vm->sp++] = top % t2;
            vm->ip++;
        } break;
        case OPT_LT: {
            if (vm->sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "< requires at least two values on the stack.\n");

            int top = vm->stack[--vm->sp];
            int t2 = vm->stack[--vm->sp];
            vm->stack[vm->sp++] = top < t2;
            vm->ip++;
        } break;
        case OPT_GT: {
            if (vm->sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "> requires at least two values on the stack.\n");

            int top = vm->stack[--vm->sp];
            int t2 = vm->stack[--vm->sp];
            vm->stack[vm->sp++] = top > t2;
            vm->ip++;
        } break;
        case OPT_EQ: {
            if (vm->sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "= requires at least two values on the stack.\n");

            int top = vm->stack[--vm->sp];
            int t2 = vm->stack[--vm->sp];
            vm->stack[vm->sp++] = top == t2;
            vm->ip++;
        } break;
        case OPT_IDENT: {
            if (op.op == PUTD) {
                if (vm->sp - 1 < 0)
                    error(ERR_UNDERFLOW, op, "`putd` requires at least one value on the stack.\n");

                vm->ip++;

                if (gen) {
                    snprintf(snbuf, 256, "\tcall $printf(l $putd_fmt, ..., w %d)\n", vm->stack[--vm->sp]);
                    strb_append(&code, snbuf);
                } else {
                    printf("%d", vm->stack[--vm->sp]);
                }
            } else if (op.op == PUTC) {
                if (vm->sp - 1 < 0)
                    error(ERR_UNDERFLOW, op, "`putc` requires at least one value on the stack.\n");

                if (gen) {
                    snprintf(snbuf, 256, "\tcall $printf(l $putc_fmt, ..., w %d)\n", vm->stack[--vm->sp]);
                    strb_append(&code, snbuf);
                } else {
                    printf("%c", vm->stack[--vm->sp]);
                }
                vm->ip++;
            } else if (op.op == LOOP) {
                assert(!gen && "GenIR not implemented");
                vm->ip++;
            } else if (op.op == END) {
                assert(!gen && "GenIR not implemented");
                if (DEBUG)
                    printf("Jumping to %s:%d:%d\n", program.data[vm->ip].loc.path, program.data[vm->ip].loc.row, program.data[vm->ip].loc.col);

                for (int i = 0; i < loops.cnt; i++) {
                    if (loops.data[i].endIp == vm->ip) {
                        vm->ip = loops.data[i].loopIp;
                        break;
                    }
                }
            } else if (op.op == DO) {
                assert(!gen && "GenIR not implemented");
                if (vm->sp - 1 < 0)
                    error(ERR_UNDERFLOW, op, "`do` requires at least one value on the stack.\n");

                int top = vm->stack[--vm->sp];
                if (top) {
                    vm->ip++;
                } else {
                    for (int i = 0; i < loops.cnt; i++) {
                        if (loops.data[i].doIp == vm->ip) {
                            vm->ip = loops.data[i].endIp + 1;
                            break;
                        }
                    }
                }
            } else if (op.op == PRINTLN || op.op == PRINT) {
                // TODO: Check existance of string in table
                if (stringTable.cnt == 0)
                    error(ERR_NO_STR, op, "`println` or `print` requires an string index. Be sure to push a string before using it.\n");

                if (vm->sp - 1 < 0)
                    error(ERR_UNDERFLOW, op, "`println` or `print` requires at least one value on the stack.\n");

                int strIdx = vm->stack[--vm->sp];
                if (op.op == PRINTLN)
                    printf("%.*s\n", (int)stringTable.data[strIdx].len, stringTable.data[strIdx].cstr);
                else
                    printf("%.*s", (int)stringTable.data[strIdx].len, stringTable.data[strIdx].cstr);
                vm->ip++;

                if (gen) {
                    snprintf(snbuf, 256, "\tcall $printf(l $str_%d, ...)\n", strIdx);
                    strb_append(&code, snbuf);
                }
            } else if (op.op == IF) {
                if (vm->sp - 1 < 0)
                    error(ERR_UNDERFLOW, op, "`if` requires at least one value on the stack.\n");

                int top = vm->stack[--vm->sp];
                if (top)
                    vm->ip++;
                else {
                    for (int i = 0; i < ifs.cnt; i++) {
                        if (ifs.data[i].ifIp == vm->ip && ifs.data[i].elseIp != -1) {
                            vm->ip = ifs.data[i].elseIp + 1;
                            break;
                        } else if (ifs.data[i].ifIp == vm->ip) {
                            vm->ip = ifs.data[i].endIp;
                            break;
                        }
                    }
                }
            } else if (op.op == ELSE) {
                for (int i = 0; i < ifs.cnt; i++) {
                    if (ifs.data[i].elseIp == vm->ip) {
                        vm->ip = ifs.data[i].endIp;
                        break;
                    }
                }
            } else if (op.op == ENDIF) {
                vm->ip++;
            } else {
                assert(!gen && "GenIR not implemented");
                printf("Unhandled intrinsic %d\n", op.op);
                // FIXME: Unhandled memory before exit? Let it leek?
                exit(1);
            }
        } break;
        case OPT_DUMP: {
            printf("> Stack Dump:\n");
            for (int j = 0; j < vm->sp; j++) {
                printf("[%d] %d\n", j, vm->stack[j]);
            }
            printf("< End Stack Dump.\n");
            if (program.data[vm->ip + 1].type == OPT_BDUMP)
                exit(1);
            vm->ip++;
        } break;
        case OPT_BDUMP: {
            assert(!gen && "GenIR not implemented");
            printf("> Back Stack Dump:\n");
            for (int j = 0; j < vm->sp; j++) {
                printf("[%d] %d\n", j, vm->backStack[j]);
            }
            printf("< End Back Stack Dump.\n");
            vm->ip++;
        } break;
        case OPT_DUP: {
            if (vm->sp - 1 < 0)
                error(ERR_UNDERFLOW, op, ". requires at least one value on the stack.\n");

            vm->stack[vm->sp] = vm->stack[vm->sp - 1];
            vm->sp++;
            vm->ip++;
        } break;
        case OPT_2DUP: {
            if (vm->sp - 2 < 0)
                error(ERR_UNDERFLOW, op, ": requires at least two value on the stack.\n");

            vm->stack[vm->sp] = vm->stack[vm->sp - 2];
            vm->stack[vm->sp + 1] = vm->stack[vm->sp - 1];
            vm->sp += 2;
            vm->ip++;
        } break;
        case OPT_DROP: {
            if (vm->sp - 1 < 0)
                error(ERR_UNDERFLOW, op, ", requires at least one value on the stack.\n");

            vm->sp--;
            vm->ip++;
        } break;
        case OPT_SWAP: {
            if (vm->sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "; requires at least one value on the stack.\n");

            int top = vm->stack[--vm->sp];
            int t2 = vm->stack[--vm->sp];
            vm->stack[vm->sp++] = top;
            vm->stack[vm->sp++] = t2;
            vm->ip++;
        } break;
        case OPT_STASH: {
            if (vm->sp - 1 < 0)
                error(ERR_UNDERFLOW, op, "<- requires at least one value on the stack.\n");

            int top = vm->stack[--vm->sp];
            if (vm->sp - top < 0)
                error(ERR_UNDERFLOW, op, "Trying to stash %d values on the stack, but only %d are available.\n", top, vm->sp);
            if (vm->bsp + top >= MAX_STACK)
                error(ERR_OVERFLOW, op, "Can't stash %d values on back stack.\n", top);

            for (int n = 0; n < top; n++)
                vm->backStack[vm->bsp++] = vm->stack[--vm->sp];
            vm->ip++;
        } break;
        case OPT_POP: {
            if (vm->sp - 1 < 0)
                error(ERR_UNDERFLOW, op, "-> requires at least one value on the stack.\n");

            int top = vm->stack[--vm->sp];
            if (vm->bsp - top < 0)
                error(ERR_UNDERFLOW, op, "Trying to pop %d values from the back stack, but only %d are available.\n", top, vm->sp);
            if (vm->sp + top >= MAX_STACK)
                error(ERR_OVERFLOW, op, "Can't pop %d values on stack.\n", top);

            for (int n = 0; n < top; n++)
                vm->stack[vm->sp++] = vm->backStack[--vm->bsp];
            vm->ip++;
        } break;
        default:
            printf("Unhandled op %s\n", optypeCStr(op.type));
            exit(1);
        }
    }

    strb_append(&code, "\tret 0\n}\n");

    if (vm->sp != 0) {
        // TODO: Move this to error() ?
        printf("E: Unhandled data on the stack.\n");
        for (int i = vm->sp - 1; i >= 0; i--) {
            printf("[%d] %d\n", i, vm->stack[i]);
        }
    }

    strb_append(&data, code.str);

    FILE *out = fopen("./output.ssa", "w");
    fwrite(data.str, sizeof(char), data.cnt, out);
    fclose(out);

    if (system("qbe ./output.ssa -o output.s") != 0) {
        printf("CompilerError: qbe return non zero.\n");
        goto defer;
    }

    if (system("clang -o output output.s") != 0) {
        printf("CompilerError: clang return non zero.\n");
    }

defer:
    VEC_FREE(loops);
    VEC_FREE(ifs);
    strb_free(code);
    strb_free(data);
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

    Tokens tokens = {0};

    bench b = {0};
    BENCH_START(&b);
    tokenize(path, code, len, &tokens);
    MEASURE(&b, "Tokenizer");

    Prog p = {0};
    parseToOPs(tokens, &p);

    free(code);

    //    BENCH_START(&b);
    //
    //    tokenize(path, code, len);
    //    MEASURE(&b, "Tokenize");
    //
    //    if (DEBUG) {
    //        for (int i = 0; i < vm.program.cnt; i++) {
    //            printf("[%d] OP: %s\n", i, optypeCStr(vm.program.data[i].type));
    //            if (vm.program.data[i].type == OPT_IDENT)
    //                printf("    > operand: %s\n", identCstr(vm.program.data[i].op));
    //            else
    //                printf("    > operand: %d\n", vm.program.data[i].op);
    //            printf("    > loc: %s:%d:%d\n", vm.program.data[i].loc.path, vm.program.data[i].loc.row, vm.program.data[i].loc.col);
    //        }
    //        printf("================================\n");
    //    }
    //
    //    BENCH_START(&b);
    //    controlFlowLink();
    //    MEASURE(&b, "ControlFlowLink");
    //
    //    if (DEBUG) {
    //        for (int i = 0; i < loops.cnt; i++) {
    //            printf("Loop: %d Do: %d End: %d\n", loops.data[i].loopIp, loops.data[i].doIp, loops.data[i].endIp);
    //        }
    //    }
    //
    //    BENCH_START(&b);
    //    interpet(&vm, vm.program, gen);
    //    MEASURE(&b, "Interpet");
    //
    //    free(code);
    //    VEC_FREE(vm.program);
    //
    //    for (int i = 0; i < stringTable.cnt; i++) {
    //        free(stringTable.data[i].cstr);
    //    }
    //
    //    VEC_FREE(stringTable);

    return 0;
}
