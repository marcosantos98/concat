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

typedef enum {
    OPT_NOP = 0,
    OPT_PLUS,
    OPT_MINUS,
    OPT_MULT,
    OPT_DIV,
    OPT_MOD,
    OPT_LT,
    OPT_GT,
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
    size_t len;
} String;

typedef struct {
    String *data;
    int cnt;
    int cap;
} Strings;

typedef struct {
    const char *path;
    int col;
    int row;
} Loc;

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

typedef struct {
    OP *data;
    int cnt;
    int cap;
} Program;

typedef struct {
    Program program;
    int ip;
} VM;

static VM vm = {0};
static Strings stringTable = {0};

size_t parseLiteralNumber(const char *code, size_t cursor, strb *s) {

    size_t end = cursor;

    while (isdigit(code[end])) {
        strb_append_single(s, code[end]);
        end++;
    }

    strb_append_single(s, 0);

    return end;
}

size_t parseIdent(const char *code, size_t cursor, strb *s) {

    size_t end = cursor;

    while (isalpha(code[end])) {
        strb_append_single(s, code[end]);
        end++;
    }

    strb_append_single(s, 0);

    return end;
}

size_t parseComment(const char *code, size_t cursor) {
    size_t end = cursor;
    while (code[end] != 10)
        end++;
    return end;
}

size_t parseStr(const char *code, size_t *cursor, strb *s) {
    size_t start = *cursor;
    size_t end = *cursor;
    end++;
    while (code[end] != '"') {
        strb_append_single(s, code[end]);
        end++;
    }

    end++; // close "

    *cursor = end;

    return end - start;
}

void tokenize(const char *path, const char *code, size_t len) {
    size_t cursor = 0;

    strb tmp = strb_init(1024);

    int col = 1;
    int row = 1;

    while (cursor < len) {
        // printf("Parsing %c at %s:%d:%d\n", code[cursor], path, row, col);
        switch (code[cursor]) {
        case '"': {
            String s = {0};
            s.len = parseStr(code, &cursor, &tmp);
            s.cstr = malloc(sizeof(char) * s.len);
            memcpy(s.cstr, tmp.str, s.len);
            tmp.cnt = 0;
            VEC_ADD(&stringTable, s);

            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_LIT_STR(stringTable.cnt - 1, l));

            col += s.len;
        } break;
        case '+': {
            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_PLUS(l));
            cursor++;
            col++;
        } break;
        case '-': {
            if (code[cursor + 1] == '>') {
                Loc l = LOC(path, col, row);
                VEC_ADD(&vm.program, OP_POP(l));
                cursor += 2;
                col += 2;
            } else {
                Loc l = LOC(path, col, row);
                VEC_ADD(&vm.program, OP_MINUS(l));
                cursor++;
                col++;
            }
        } break;
        case '*': {
            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_MULT(l));
            cursor++;
            col++;
        } break;
        case '/': {
            if (code[cursor + 1] == '/') {
                size_t old = cursor;
                cursor = parseComment(code, cursor);
                col += cursor - old;
            } else {

                Loc l = LOC(path, col, row);
                VEC_ADD(&vm.program, OP_DIV(l));
                cursor++;
                col++;
            }
        } break;
        case '%': {
            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_MOD(l));
            cursor++;
            col++;
        } break;
        case '<': {
            if (code[cursor + 1] == '-') {
                Loc l = LOC(path, col, row);
                VEC_ADD(&vm.program, OP_STASH(l));
                cursor += 2;
                col += 2;
            } else {
                Loc l = LOC(path, col, row);
                VEC_ADD(&vm.program, OP_LT(l));
                cursor++;
                col++;
            }
        } break;
        case '>': {
            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_GT(l));
            cursor++;
            col++;
        } break;
        case '?': {
            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_DUMP(l));
            cursor++;
            col++;
        } break;
        case '!': {
            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_BDUMP(l));
            cursor++;
            col++;
        } break;
        case '.': {
            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_DUP(l));
            cursor++;
            col++;
        } break;
        case ':': {
            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_2DUP(l));
            cursor++;
            col++;
        } break;
        case ',': {
            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_DROP(l));
            cursor++;
            col++;
        } break;
        case ';': {
            Loc l = LOC(path, col, row);
            VEC_ADD(&vm.program, OP_SWAP(l));
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
                cursor = parseLiteralNumber(code, cursor, &tmp);
                Loc l = LOC(path, col, row);
                OP op = OP_LIT_NUMBER(atoi(tmp.str), l);
                VEC_ADD(&vm.program, op);
                col += tmp.cnt - 1; // Don't include the NULL terminator
                tmp.cnt = 0;
            } else if (isalpha(code[cursor])) {
                cursor = parseIdent(code, cursor, &tmp);

                Loc l = LOC(path, col, row);
                if (strncmp("sout", tmp.str, 4) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(PUTD, l));
                } else if (strncmp("loop", tmp.str, 4) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(LOOP, l));
                } else if (strncmp("end", tmp.str, 3) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(END, l));
                } else if (strncmp("do", tmp.str, 2) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(DO, l));
                } else if (strncmp("putc", tmp.str, 4) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(PUTC, l));
                } else if (strncmp("println", tmp.str, 7) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(PRINTLN, l));
                } else if (strncmp("print", tmp.str, 5) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(PRINT, l));
                } else {
                    printf("Ident not handled! %s\n", tmp.str);
                    exit(1);
                }
                col += tmp.cnt - 1;
                tmp.cnt = 0;
            } else {
                printf("Char not handled %c\n", code[cursor]);
                exit(1);
            }
        }
        }
    }

    Loc l = LOC(path, col, row);
    VEC_ADD(&vm.program, OP_NOP(l));
    strb_free(tmp);
}

#define MAX_STACK 100

typedef struct {
    int *data;
    int cnt;
    int cap;
} dos;

#include <stdarg.h>

typedef enum {
    ERR_OVERFLOW,
    ERR_UNDERFLOW,
    ERR_UNCLOSED_LOOP,
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

void interpet() {

    int sp = 0;
    int stack[MAX_STACK] = {0};
    int bsp = 0;
    int backStack[MAX_STACK] = {0};

    dos d = {0};

    while (vm.program.data[vm.ip].type != OPT_NOP) {
        OP op = vm.program.data[vm.ip];
        switch (op.type) {
        case OPT_LIT_NUMBER:
        case OPT_LIT_STR: {
            if (sp + 1 >= MAX_STACK)
                error(ERR_OVERFLOW, op, "Can't push literal number %d\n", op.op);

            stack[sp++] = op.op;
            vm.ip++;
        } break;
        case OPT_PLUS: {
            if (sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "+ requires at least two values on the stack.\n");

            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top + t2;
            vm.ip++;
        } break;
        case OPT_MINUS: {
            if (sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "- requires at least two values on the stack.\n");

            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top - t2;
            vm.ip++;
        } break;
        case OPT_MULT: {
            if (sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "* requires at least two values on the stack.\n");

            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top * t2;
            vm.ip++;
        } break;
        case OPT_DIV: {
            if (sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "/ requires at least two values on the stack.\n");

            int top = stack[--sp];
            int t2 = stack[--sp];
            // TODO: Check for division by zero
            stack[sp++] = top / t2;
            vm.ip++;
        } break;
        case OPT_MOD: {
            if (sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "% requires at least two values on the stack.\n");

            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top % t2;
            vm.ip++;
        } break;
        case OPT_LT: {
            if (sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "< requires at least two values on the stack.\n");

            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top < t2;
            vm.ip++;
        } break;
        case OPT_GT: {
            if (sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "> requires at least two values on the stack.\n");

            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top > t2;
            vm.ip++;
        } break;
        case OPT_IDENT: {
            if (op.op == PUTD) {
                if (sp - 1 < 0)
                    error(ERR_UNDERFLOW, op, "`putd` requires at least one value on the stack.\n");

                printf("%d", stack[--sp]);
                vm.ip++;
            } else if (op.op == PUTC) {
                if (sp - 1 < 0)
                    error(ERR_UNDERFLOW, op, "`putc` requires at least one value on the stack.\n");

                printf("%c", stack[--sp]);
                vm.ip++;
            } else if (op.op == LOOP) {

                VEC_ADD(&d, vm.ip);
                vm.ip++;

                // if (d.cnt == 0)
                //     error(ERR_NO_DO, op, "`do` keyword is required before `loop`.\n");

                // int end = vm.ip;
                // while (true) {
                //     if (vm.program.data[end].type == OPT_NOP)
                //         error(ERR_UNCLOSED_LOOP, op, "loop requires an `end` keyword.\n");
                //     if (vm.program.data[end].type == OPT_IDENT && vm.program.data[end].op == 2)
                //         break;
                //     end++;
                // }
                // if (sp - 1 < 0)
                //     error(ERR_UNDERFLOW, op, "`loop` requires at least one value on the stack.\n");

                // int top = stack[--sp];
                // if (top)
                //     vm.ip++;
                // else
                //     vm.ip = end + 1;
            } else if (op.op == END) {
                if (d.cnt == 0)
                    error(ERR_NO_DO, op, "`end` doesn't have a jump location.\n");
                vm.ip = d.data[--d.cnt];
            } else if (op.op == DO) {
                if (sp - 1 < 0)
                    error(ERR_UNDERFLOW, op, "`do` requires at least one value on the stack.\n");

                int top = stack[--sp];
                if (top) {
                    vm.ip++;
                } else {
                    int end = vm.ip;
                    while (true) {
                        if (vm.program.data[end].type == OPT_IDENT && vm.program.data[end].op == 2)
                            break;
                        end++;
                    }
                    vm.ip = end + 1;
                    d.cnt--;
                }
            } else if (op.op == PRINTLN || op.op == PRINT) {
                // TODO: Check existance of string in table
                if (stringTable.cnt == 0)
                    error(ERR_NO_STR, op, "`println` or `print` requires an string index. Be sure to push a string before using it.\n");

                if (sp - 1 < 0)
                    error(ERR_UNDERFLOW, op, "`println` or `print` requires at least one value on the stack.\n");

                int strIdx = stack[--sp];
                if (op.op == PRINTLN)
                    printf("%.*s\n", (int)stringTable.data[strIdx].len, stringTable.data[strIdx].cstr);
                else
                    printf("%.*s", (int)stringTable.data[strIdx].len, stringTable.data[strIdx].cstr);
                stringTable.cnt--;
                vm.ip++;
            } else {
                printf("Unhandled intrinsic %d\n", op.op);
                // FIXME: Unhandled memory before exit? Let it leek?
                exit(1);
            }
        } break;
        case OPT_DUMP: {
            printf("> Stack Dump:\n");
            for (int j = 0; j < sp; j++) {
                printf("[%d] %d\n", j, stack[j]);
            }
            printf("< End Stack Dump.\n");
            vm.ip++;
        } break;
        case OPT_BDUMP: {
            printf("> Back Stack Dump:\n");
            for (int j = 0; j < bsp; j++) {
                printf("[%d] %d\n", j, backStack[j]);
            }
            printf("< End Back Stack Dump.\n");
            vm.ip++;
        } break;
        case OPT_DUP: {
            if (sp - 1 < 0)
                error(ERR_UNDERFLOW, op, ". requires at least one value on the stack.\n");

            stack[sp] = stack[sp - 1];
            sp++;
            vm.ip++;
        } break;
        case OPT_2DUP: {
            if (sp - 2 < 0)
                error(ERR_UNDERFLOW, op, ": requires at least two value on the stack.\n");

            stack[sp] = stack[sp - 2];
            stack[sp + 1] = stack[sp - 1];
            sp += 2;
            vm.ip++;
        } break;
        case OPT_DROP: {
            if (sp - 1 < 0)
                error(ERR_UNDERFLOW, op, ", requires at least one value on the stack.\n");

            sp--;
            vm.ip++;
        } break;
        case OPT_SWAP: {
            if (sp - 2 < 0)
                error(ERR_UNDERFLOW, op, "; requires at least one value on the stack.\n");

            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top;
            stack[sp++] = t2;
            vm.ip++;
        } break;
        case OPT_STASH: {
            if (sp - 1 < 0)
                error(ERR_UNDERFLOW, op, "<- requires at least one value on the stack.\n");

            int top = stack[--sp];
            if (sp - top < 0)
                error(ERR_UNDERFLOW, op, "Trying to stash %d values on the stack, but only %d are available.\n", top, sp);
            if (bsp + top >= MAX_STACK)
                error(ERR_OVERFLOW, op, "Can't stash %d values on back stack.\n", top);

            for (int n = 0; n < top; n++)
                backStack[bsp++] = stack[--sp];
            vm.ip++;
        } break;
        case OPT_POP: {
            if (sp - 1 < 0)
                error(ERR_UNDERFLOW, op, "-> requires at least one value on the stack.\n");

            int top = stack[--sp];
            if (bsp - top < 0)
                error(ERR_UNDERFLOW, op, "Trying to pop %d values from the back stack, but only %d are available.\n", top, bsp);
            if (sp + top >= MAX_STACK)
                error(ERR_OVERFLOW, op, "Can't pop %d values on stack.\n", top);

            for (int n = 0; n < top; n++)
                stack[sp++] = backStack[--bsp];
            vm.ip++;
        } break;
        default:
            printf("Unhandled op %s\n", optypeCStr(op.type));
            exit(1);
        }
    }

    if (sp != 0) {
        printf("E: Unhandled data on the stack.\n");
        for (int i = sp - 1; i > 0; i--) {
            printf("[%d] %d\n", i, stack[i]);
        }
    }

    VEC_FREE(d);
}

int main(int argc, char **argv) {

    if (argc != 2)
        return 1;

    _ *argv++;

    const char *path = *argv++;

    size_t len;
    char *code = read_file(path, &len);
    if (code == NULL)
        return 1;

    bench b = {0};
    BENCH_START(&b);

    tokenize(path, code, len);
    MEASURE(&b, "Tokenize");

    if (DEBUG) {
        for (int i = 0; i < vm.program.cnt; i++) {
            printf("[%d] OP: %s\n", i, optypeCStr(vm.program.data[i].type));
            printf("    > operand: %d\n", vm.program.data[i].op);
            printf("    > loc: %s:%d:%d\n", vm.program.data[i].loc.path, vm.program.data[i].loc.row, vm.program.data[i].loc.col);
        }
        printf("================================\n");
    }

    BENCH_START(&b);
    interpet();
    MEASURE(&b, "Interpet");

    free(code);
    VEC_FREE(vm.program);

    for (int i = 0; i < stringTable.cnt; i++) {
        free(stringTable.data[i].cstr);
    }

    VEC_FREE(stringTable);

    return 0;
}
