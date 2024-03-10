#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <io.h>
#include <strb.h>
#include <vector.h>

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
    OPT_IDENT,
    OPT_DUMP,
    OPT_DUP,
    OPT_DROP,
    OPT_SWAP,
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
    case OPT_IDENT:
        return "OPT_IDENT";
    case OPT_DUMP:
        return "OPT_DUMP";
    case OPT_DUP:
        return "OPT_DUP";
    case OPT_DROP:
        return "OPT_DROP";
    case OPT_SWAP:
        return "OPT_SWAP";
    default:
        return "Unknown type";
    }
}

typedef struct {
    OPType type;
    int op;
} OP;

#define OP_NOP \
    (OP) { .type = OPT_NOP }
#define OP_PLUS \
    (OP) { .type = OPT_PLUS }
#define OP_MINUS \
    (OP) { .type = OPT_MINUS }
#define OP_MULT \
    (OP) { .type = OPT_MULT }
#define OP_DIV \
    (OP) { .type = OPT_DIV }
#define OP_MOD \
    (OP) { .type = OPT_MOD }
#define OP_LT \
    (OP) { .type = OPT_LT }
#define OP_GT \
    (OP) { .type = OPT_GT }
#define OP_LIT_NUMBER(o) \
    (OP) { .type = OPT_LIT_NUMBER, .op = o }
#define OP_IDENT(o) \
    (OP) { .type = OPT_IDENT, .op = o }
#define OP_DUMP \
    (OP) { .type = OPT_DUMP }
#define OP_DUP \
    (OP) { .type = OPT_DUP }
#define OP_DROP \
    (OP) { .type = OPT_DROP }
#define OP_SWAP \
    (OP) { .type = OPT_SWAP }

#define SOUT 0
#define LOOP 1
#define END 2
#define DO 3
#define PUTC 4

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

void tokenize(const char *code, size_t len) {
    size_t cursor = 0;

    strb tmp = strb_init(1024);

    while (cursor < len) {
        switch (code[cursor]) {
        case '+': {
            VEC_ADD(&vm.program, OP_PLUS);
            cursor++;
        } break;
        case '-': {
            VEC_ADD(&vm.program, OP_MINUS);
            cursor++;
        } break;
        case '*': {
            VEC_ADD(&vm.program, OP_MULT);
            cursor++;
        } break;
        case '/': {
            if (code[cursor + 1] == '/') {
                cursor = parseComment(code, cursor);
            } else {
                VEC_ADD(&vm.program, OP_DIV);
                cursor++;
            }
        } break;
        case '%': {
            VEC_ADD(&vm.program, OP_MOD);
            cursor++;
        } break;
        case '<': {
            VEC_ADD(&vm.program, OP_LT);
            cursor++;
        } break;
        case '>': {
            VEC_ADD(&vm.program, OP_GT);
            cursor++;
        } break;
        case '?': {
            VEC_ADD(&vm.program, OP_DUMP);
            cursor++;
        } break;
        case '.': {
            VEC_ADD(&vm.program, OP_DUP);
            cursor++;
        } break;
        case ',': {
            VEC_ADD(&vm.program, OP_DROP);
            cursor++;
        } break;
        case ';': {
            VEC_ADD(&vm.program, OP_SWAP);
            cursor++;
        } break;
        case ' ':
        case '\n':
        case '\r':
        case '\t': {
            cursor++;
        } break;
        default: {
            if (isdigit(code[cursor])) {
                cursor = parseLiteralNumber(code, cursor, &tmp);
                OP op = OP_LIT_NUMBER(atoi(tmp.str));
                VEC_ADD(&vm.program, op);
                tmp.cnt = 0;
            } else if (isalpha(code[cursor])) {
                cursor = parseIdent(code, cursor, &tmp);

                if (strncmp("sout", tmp.str, 4) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(SOUT));
                } else if (strncmp("loop", tmp.str, 4) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(LOOP));
                } else if (strncmp("end", tmp.str, 3) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(END));
                } else if (strncmp("do", tmp.str, 2) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(DO));
                } else if (strncmp("putc", tmp.str, 4) == 0) {
                    VEC_ADD(&vm.program, OP_IDENT(PUTC));
                } else {
                    printf("Ident not handled! %s\n", tmp.str);
                    exit(1);
                }

                tmp.cnt = 0;
            } else {
                printf("Char not handled %c\n", code[cursor]);
                exit(1);
            }
        }
        }
    }

    VEC_ADD(&vm.program, OP_NOP);
}

#define MAX_STACK 100

typedef struct {
    int *data;
    int cnt;
    int cap;
} dos;

void interpet() {

    int sp = 0;
    int stack[MAX_STACK] = {0};

    dos d = {0};

    while (vm.program.data[vm.ip].type != OPT_NOP) {
        OP op = vm.program.data[vm.ip];
        switch (op.type) {
        case OPT_LIT_NUMBER: {
            stack[sp++] = op.op;
            vm.ip++;
        } break;
        case OPT_PLUS: {
            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top + t2;
            vm.ip++;
        } break;
        case OPT_MINUS: {
            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top - t2;
            vm.ip++;
        } break;
        case OPT_MULT: {
            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top * t2;
            vm.ip++;
        } break;
        case OPT_DIV: {
            int top = stack[--sp];
            int t2 = stack[--sp];
            // TODO: Check for division by zero
            stack[sp++] = top / t2;
            vm.ip++;
        } break;
        case OPT_MOD: {
            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top % t2;
            vm.ip++;
        } break;
        case OPT_LT: {
            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top < t2;
            vm.ip++;
        } break;
        case OPT_GT: {
            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top > t2;
            vm.ip++;
        } break;
        case OPT_IDENT: {
            if (op.op == SOUT) {
                printf("%d", stack[--sp]);
                vm.ip++;
            } else if (op.op == PUTC) {
                printf("%c", stack[--sp]);
                vm.ip++;
            } else if (op.op == LOOP) {
                int end = vm.ip;
                while (true) {
                    if (vm.program.data[end].type == OPT_IDENT && vm.program.data[end].op == 2)
                        break;
                    end++;
                }

                int top = stack[--sp];
                if (top)
                    vm.ip++;
                else
                    vm.ip = end + 1;
            } else if (op.op == END) {
                if (d.cnt != 0) {
                    vm.ip = d.data[--d.cnt];
                } else
                    vm.ip++;
            } else if (op.op == DO) {
                if (stack[sp - 1] != 0) {
                    VEC_ADD(&d, vm.ip);
                    vm.ip++;
                } else {
                    int end = vm.ip;
                    while (true) {
                        if (vm.program.data[end].type == OPT_IDENT && vm.program.data[end].op == 2)
                            break;
                        end++;
                    }
                    vm.ip = end + 1;
                }
            } else {
                printf("Unhandled intrinsic %d\n", op.op);
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
        case OPT_DUP: {
            stack[sp] = stack[sp - 1];
            sp++;
            vm.ip++;
        } break;
        case OPT_DROP: {
            sp--;
            vm.ip++;
        } break;
        case OPT_SWAP: {
            int top = stack[--sp];
            int t2 = stack[--sp];
            stack[sp++] = top;
            stack[sp++] = t2;
            vm.ip++;
        } break;
        default:
            printf("Unhandled op %s\n", optypeCStr(op.type));
            exit(1);
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
    tokenize(code, len);

    for (int i = 0; i < vm.program.cnt; i++) {
        printf("[%d] OP: %s\n", i, optypeCStr(vm.program.data[i].type));
        printf("OP operand: %d\n", vm.program.data[i].op);
    }

    printf("================================\n");
    interpet();

    free(code);
    VEC_FREE(vm.program);

    return 0;
}
