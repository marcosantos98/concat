#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <io.h>
#include <strb.h>
#include <vector.h>

#define _ (void)

typedef enum {
    OPT_PLUS,
    OPT_MINUS,
    OPT_MULT,
    OPT_LIT_NUMBER,
    OPT_IDENT,
    OPT_DUMP,
} OPType;

char *optypeCStr(OPType op) {
    switch (op) {
    case OPT_PLUS:
        return "OPT_PLUS";
    case OPT_MINUS:
        return "OPT_MINUS";
    case OPT_MULT:
        return "OPT_MULT";
    case OPT_LIT_NUMBER:
        return "OPT_LIT_NUMBER";
    case OPT_IDENT:
        return "OPT_IDENT";
    case OPT_DUMP:
        return "OPT_DUMP";
    default:
        return "Unknown type";
    }
}

typedef struct {
    OPType type;
    int op;
} OP;

#define OP_PLUS \
    (OP) { .type = OPT_PLUS }
#define OP_MINUS \
    (OP) { .type = OPT_MINUS }
#define OP_MULT \
    (OP) { .type = OPT_MULT }
#define OP_LIT_NUMBER(o) \
    (OP) { .type = OPT_LIT_NUMBER, .op = o }
#define OP_IDENT(o) \
    (OP) { .type = OPT_IDENT, .op = o }
#define OP_DUMP \
    (OP) { .type = OPT_DUMP }

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
        case '?': {
            VEC_ADD(&vm.program, OP_DUMP);
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
                    VEC_ADD(&vm.program, OP_IDENT(0));
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
}

#define MAX_STACK 100

void interpet() {

    int i = 0;

    int stack[MAX_STACK] = {0};

    while (i < vm.program.cnt) {
        OP op = vm.program.data[i];
        switch (op.type) {
        case OPT_LIT_NUMBER: {
            stack[vm.ip++] = op.op;
            i++;
        } break;
        case OPT_PLUS: {
            int top = stack[--vm.ip];
            int t2 = stack[--vm.ip];
            stack[vm.ip++] = top + t2;
            i++;
        } break;
        case OPT_MINUS: {
            int top = stack[--vm.ip];
            int t2 = stack[--vm.ip];
            stack[vm.ip++] = top - t2;
            i++;
        } break;
        case OPT_MULT: {
            int top = stack[--vm.ip];
            int t2 = stack[--vm.ip];
            stack[vm.ip++] = top * t2;
            i++;
        } break;
        case OPT_IDENT: {
            if (op.op == 0) {
                // sout
                printf("%d\n", stack[--vm.ip]);
                i++;
            } else {
                printf("Unhandled intrinsic %d\n", op.op);
                exit(1);
            }
        } break;
        case OPT_DUMP: {
            for (int j = 0; j < vm.ip; j++) {
                printf("[%d] %d\n", j, stack[j]);
            }
            i++;
        } break;
        default:
            printf("Unhandled op %s\n", optypeCStr(op.type));
            exit(1);
        }
    }
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
        printf("OP: %s\n", optypeCStr(vm.program.data[i].type));
        printf("OP operand: %d\n", vm.program.data[i].op);
    }

    printf("================================\n");
    interpet();

    free(code);
    VEC_FREE(vm.program);

    return 0;
}
