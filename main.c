#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bench.h>
#include <io.h>
#include <strb.h>
#include <vector.h>

#define _ (void)

#define List(name, type) \
    typedef struct {     \
        type *data;      \
        int cnt;         \
        int cap;         \
    } name

#ifdef DEBUG
#define MEASURE(bPtr, strmsg)        \
    do {                             \
        BENCH_MEASURE(bPtr, strmsg); \
    } while (0)
#else
#define MEASURE(bPtr, strmsg)
#endif

#define KB 1024

static char temp_buf[8 * KB] = {0};
static size_t temp_ptr = 0;

size_t cstr_cpy(void *src, size_t len) {
    size_t at = temp_ptr;
    memcpy(temp_buf + temp_ptr, src, len);
    temp_ptr += len + 1;
    return at;
}

bool str_eq(size_t ptr, const char *to_check) {
    char *at_temp = temp_buf + ptr;
    size_t temp_len = strlen(at_temp);
    size_t to_check_len = strlen(to_check);
    if (temp_len != to_check_len) {
        return false;
    }
    return strncmp(at_temp, to_check, to_check_len) == 0;
}

#define CSTR(ptr) temp_buf + (ptr)

// :tokenizer
typedef enum {
    TT_PLUS = 0,
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
    TT_COUNT,
} TokenType;
static_assert(TT_COUNT == 19, "Implement newly add TokenType");

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
    TokenType t;
    Loc l;
    size_t lit_ptr;
    int index;
} Token;

List(Tokens, Token);

Token make_token(TokenType t, size_t temp_ptr, Loc l, int index) {
    return (Token){t, l, temp_ptr, index};
}

size_t tokenize_identifier(char *const code, size_t code_len, size_t cursor) {
    size_t end = cursor;

    while (end < code_len && (code[end] != ' ' && code[end] != '\n'))
        end++;

    return end;
}

size_t tokenize_str_literal(char *const code, size_t len, size_t cursor) {
    size_t end = ++cursor;

    while (end < len && code[end] != '"')
        end++;

    return end + 1;
}

size_t tokenize_number_literal(char *const code, size_t len, size_t cursor) {
    size_t end = cursor;

    while (end < len && isdigit(code[end]) && (code[end] != ' ' && code[end] != '\n'))
        end++;

    return end;
}

size_t tokenize_comment(const char *code, size_t cursor) {
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
            cursor += 1;
            size_t end = tokenize_str_literal(code, len, cursor);
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_LIT_STR, cstr_cpy(code + cursor, end - cursor - 1), l, tokens->cnt));
            col += end - cursor;
            cursor = end;
        } break;
        case '+': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_PLUS, cstr_cpy(code + cursor, 1), l, tokens->cnt));
            cursor++;
            col++;
        } break;
        case '-': {
            if (code[cursor + 1] == '>') {
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, make_token(TT_POP, cstr_cpy(code + cursor, 2), l, tokens->cnt));
                cursor += 2;
                col += 2 - 1;
            } else {
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, make_token(TT_MINUS, cstr_cpy(code + cursor, 1), l, tokens->cnt));
                cursor++;
                col++;
            }
        } break;
        case '*': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_MULT, cstr_cpy(code + cursor, 1), l, tokens->cnt));
            cursor++;
            col++;
        } break;
        case '/': {
            if (code[cursor + 1] == '/') {
                size_t old = cursor;
                cursor = tokenize_comment(code, cursor);
                col += cursor - old;
            } else {
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, make_token(TT_DIV, cstr_cpy(code + cursor, 1), l, tokens->cnt));
                cursor++;
                col++;
            }
        } break;
        case '%': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_MOD, cstr_cpy(code + cursor, 1), l, tokens->cnt));
            cursor++;
            col++;
        } break;
        case '<': {
            if (code[cursor + 1] == '-') {
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, make_token(TT_STASH, cstr_cpy(code + cursor, 2), l, tokens->cnt));
                cursor += 2;
                col += 2;
            } else {
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, make_token(TT_LT, cstr_cpy(code + cursor, 1), l, tokens->cnt));
                cursor++;
                col++;
            }
        } break;
        case '>': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_GT, cstr_cpy(code + cursor, 1), l, tokens->cnt));
            cursor++;
            col++;
        } break;
        case '=': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_EQ, cstr_cpy(code + cursor, 1), l, tokens->cnt));
            cursor++;
            col++;
        } break;
        case '?': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_DUMP, cstr_cpy(code + cursor, 1), l, tokens->cnt));
            cursor++;
            col++;
        } break;
        case '!': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_BDUMP, cstr_cpy(code + cursor, 1), l, tokens->cnt));
            cursor++;
            col++;
        } break;
        case '.': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_DUP, cstr_cpy(code + cursor, 1), l, tokens->cnt));
            cursor++;
            col++;
        } break;
        case ':': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_2DUP, cstr_cpy(code + cursor, 1), l, tokens->cnt));
            cursor++;
            col++;
        } break;
        case ',': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_DROP, cstr_cpy(code + cursor, 1), l, tokens->cnt));
            cursor++;
            col++;
        } break;
        case ';': {
            Loc l = LOC(path, col, row);
            VEC_ADD(tokens, make_token(TT_SWAP, cstr_cpy(code + cursor, 1), l, tokens->cnt));
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
                size_t end = tokenize_number_literal(code, len, cursor);
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, make_token(TT_LIT_NUMBER, cstr_cpy(code + cursor, end - cursor), l, tokens->cnt));
                col += end - cursor;
                cursor = end;
            } else if (isalpha(code[cursor])) {
                size_t end = tokenize_identifier(code, len, cursor);
                Loc l = LOC(path, col, row);
                VEC_ADD(tokens, make_token(TT_WORD, cstr_cpy(code + cursor, end - cursor), l, tokens->cnt));
                col += end - cursor;
                cursor = end;
            } else {
                printf("Char not handled %c\n", code[cursor]);
                exit(1);
            }
        }
        }
    }
    return true;
}
// ;tokenizer

// :parser
typedef enum {
    OP_NOP = 0,
    OP_BINOP,
    OP_LIT_NUMBER,
    OP_LIT_STR,
    OP_INTRINSIC,
    OP_DUMP,
    OP_BDUMP,
    OP_DUP,
    OP_2DUP,
    OP_DROP,
    OP_SWAP,
    OP_STASH,
    OP_POP,
    OP_COUNT,
} OpType;
static_assert(OP_COUNT == 13, "Implement newly added OpType");

typedef enum {
    BT_PLUS,
    BT_MINUS,
    BT_MULT,
    BT_DIV,
    BT_MOD,
    BT_LT,
    BT_GT,
    BT_EQ,
    BT_COUNT,
} BinopType;
static_assert(BT_COUNT == 8, "Implement newly added BinopType");

typedef enum {
    W_DEFINED = 0,
    W_PUTD,
    W_LOOP,
    W_END,
    W_MEM,
    W_W_MEM,
    W_W_MEM64,
    W_DEREF,
    W_DO,
    W_PUTC,
    W_PRINTLN,
    W_PRINT,
    W_IF,
    W_ELSE,
    W_ENDIF,
    W_AS_STR,
    W_DEF,
    W_INCLUDE,
    W_COUNT,
} IntrinsicType;
static_assert(W_COUNT == 18, "Implement newly added IntrinsicType");

typedef struct {
    Loc l;
    OpType t;
    long op;
    int link;
    int index;
} Op;

char *op_to_str(Op op) {
    static_assert(W_COUNT == 18, "Implement newly added IntrinsicType");
    static_assert(BT_COUNT == 8, "Implement newly added BinopType");
    static_assert(OP_COUNT == 13, "Implement newly added OpType");
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
    case OP_INTRINSIC: {
        switch (op.op) {
        case W_INCLUDE:
            return "W_INCLUDE";
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
        case W_MEM:
            return "W_MEM";
        case W_W_MEM:
            return "W_W_MEM";
        case W_W_MEM64:
            return "W_W_MEM64";
        case W_DEREF:
            return "W_DEREF";
        case W_AS_STR:
            return "W_AS_STR";
        case W_DEF:
            return "W_DEF";
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

List(Program, Op);
List(UStrList, size_t);

#define MAX_MEMORY 64 * KB
#define MAX_DEFINED 1000
typedef struct {
    Program prog;

    UStrList definedTable;
    int definedData[MAX_DEFINED];

    int constData[MAX_DEFINED];
    char mem[MAX_MEMORY];
    size_t mem_ptr;
} VM;

typedef struct {
    VM vm;
    Tokens tokens;
    char *code;
} ProgramRun;

size_t push_str_to_mem(VM *vm, size_t temp_ptr) {
    size_t len = strlen(CSTR(temp_ptr));
    memcpy(vm->mem + vm->mem_ptr, CSTR(temp_ptr), len);
    size_t at = vm->mem_ptr;
    vm->mem_ptr += len + 1;
    return at;
}

Op parse_binop(Token t) {
    static_assert(BT_COUNT == 8, "Implement newly added BinopType");
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
        return (Op){t.l, OP_NOP, 0, 0, t.index};
    }
    return (Op){t.l, OP_NOP, 0, 0, t.index};
}

#define VEC_LEN(vec) (vec).cnt

int find_defined(ProgramRun program, Op o) {
    size_t to_check = VEC_GET(program.tokens, o.index).lit_ptr;
    for (int i = 0; i < VEC_LEN(program.vm.definedTable); i++) {
        size_t it = VEC_GET(program.vm.definedTable, i);
        // printf("> %s == %s\n", CSTR(to_check), CSTR(it));
        if (str_eq(to_check, CSTR(it))) {
            return i;
        }
    }
    return -1;
}

Op parse_identifier(Token t, ProgramRun *program) {
    Op o = (Op){.l = t.l, .t = OP_INTRINSIC, .op = W_DEFINED, .link = 0, .index = t.index};

    static_assert(W_COUNT == 18, "Implement newly added IntrinsicType");
    if (str_eq(t.lit_ptr, "sout")) {
        o.op = W_PUTD;
    } else if (str_eq(t.lit_ptr, "loop")) {
        o.op = W_LOOP;
    } else if (str_eq(t.lit_ptr, "endif")) {
        o.op = W_ENDIF;
    } else if (str_eq(t.lit_ptr, "do")) {
        o.op = W_DO;
    } else if (str_eq(t.lit_ptr, "putc")) {
        o.op = W_PUTC;
    } else if (str_eq(t.lit_ptr, "println")) {
        o.op = W_PRINTLN;
    } else if (str_eq(t.lit_ptr, "print")) {
        o.op = W_PRINT;
    } else if (str_eq(t.lit_ptr, "if")) {
        o.op = W_IF;
    } else if (str_eq(t.lit_ptr, "else")) {
        o.op = W_ELSE;
    } else if (str_eq(t.lit_ptr, "end")) {
        o.op = W_END;
    } else if (str_eq(t.lit_ptr, "mem")) {
        o.op = W_MEM;
    } else if (str_eq(t.lit_ptr, "w_mem")) {
        o.op = W_W_MEM;
    } else if (str_eq(t.lit_ptr, "w64_mem")) {
        o.op = W_W_MEM64;
    } else if (str_eq(t.lit_ptr, "deref")) {
        o.op = W_DEREF;
    } else if (str_eq(t.lit_ptr, "as_str")) {
        o.op = W_AS_STR;
    } else if (str_eq(t.lit_ptr, "def")) {
        o.op = W_DEF;
    } else if (str_eq(t.lit_ptr, "include")) {
        o.op = W_INCLUDE;
    } else {
        if (find_defined(*program, o) == -1) {
            VEC_ADD(&program->vm.definedTable, t.lit_ptr);
        }
    }

    return o;
}

bool parse(ProgramRun *prog) {
    int i = 0;
    VM *vm = &prog->vm;
    static_assert(TT_COUNT == 19, "Implement newly add TokenType");
    while (i < VEC_LEN(prog->tokens)) {
        Token t = VEC_GET(prog->tokens, i);
        switch (t.t) {
        case TT_PLUS:
        case TT_MINUS:
        case TT_MULT:
        case TT_DIV:
        case TT_MOD:
        case TT_LT:
        case TT_GT:
        case TT_EQ: {
            VEC_ADD(&vm->prog, parse_binop(t));
        } break;
        case TT_LIT_NUMBER: {
            Op o = (Op){.l = t.l, .t = OP_LIT_NUMBER, .op = atoi(CSTR(t.lit_ptr)), .link = 0};
            VEC_ADD(&vm->prog, o);
        } break;
        case TT_LIT_STR: {
            Op o = (Op){.l = t.l, .t = OP_LIT_STR, .op = push_str_to_mem(vm, t.lit_ptr), .link = 0};
            VEC_ADD(&vm->prog, o);
        } break;
        case TT_WORD: {
            VEC_ADD(&vm->prog, parse_identifier(t, prog));
        } break;
        case TT_DUMP: {
            Op o = (Op){.l = t.l, .t = OP_DUMP, .op = 0, .link = 0};
            VEC_ADD(&vm->prog, o);
        } break;
        case TT_BDUMP: {
            Op o = (Op){.l = t.l, .t = OP_BDUMP, .op = 0, .link = 0};
            VEC_ADD(&vm->prog, o);
        } break;
        case TT_DUP: {
            Op o = (Op){.l = t.l, .t = OP_DUP, .op = 0, .link = 0};
            VEC_ADD(&vm->prog, o);
        } break;
        case TT_2DUP: {
            Op o = (Op){.l = t.l, .t = OP_2DUP, .op = 0, .link = 0};
            VEC_ADD(&vm->prog, o);
        } break;
        case TT_DROP: {
            Op o = (Op){.l = t.l, .t = OP_DROP, .op = 0, .link = 0};
            VEC_ADD(&vm->prog, o);
        } break;
        case TT_SWAP: {
            Op o = (Op){.l = t.l, .t = OP_SWAP, .op = 0, .link = 0};
            VEC_ADD(&vm->prog, o);
        } break;
        case TT_STASH: {
            Op o = (Op){.l = t.l, .t = OP_STASH, .op = 0, .link = 0};
            VEC_ADD(&vm->prog, o);
        } break;
        case TT_POP: {
            Op o = (Op){.l = t.l, .t = OP_POP, .op = 0, .link = 0};
            VEC_ADD(&vm->prog, o);
        } break;
        case TT_COUNT:
            break;
        }
        i++;
    }

    Loc l = VEC_GET(prog->tokens, i - 1).l;
    l.col = 1;
    l.row++;
    Op o = (Op){.l = l, .t = OP_NOP, .op = 0, .link = 0};
    VEC_ADD(&vm->prog, o);

#ifdef DEBUG
    for (int i = 0; i < vm->definedTable.cnt; i++) {
        printf("[%d] %s\n", i, CSTR(vm->definedTable.data[i]));
    }
#endif

    return false;
}
// ;parser

// :defined
bool is_intrinsic(Op o, IntrinsicType t) {
    return o.t == OP_INTRINSIC && (IntrinsicType)o.op == t;
}

#define TOKEN_LIT(prog, index) CSTR(VEC_GET((prog).tokens, (index)).lit_ptr)

void print_operations(ProgramRun prog) {
    for (int i = 0; i < VEC_LEN(prog.vm.prog); i++) {
        Op it = VEC_GET(prog.vm.prog, i);
        printf("[%d] OP: %s\n", i, op_to_str(it));
        printf("    > operand: %ld\n", it.op);
        printf("    > link: %d\n", it.link);
        printf("    > loc: ");
        printloc(it.l);
        printf("\n");
        printf("    > repr: %s\n", TOKEN_LIT(prog, it.index));
    }
}

#define FOR_LIST(list) for (int i = 0; i < (list).cnt; i++)

bool is_libc_word(ProgramRun prog, Op o) {
    return is_intrinsic(o, W_DEFINED) && (str_eq(VEC_GET(prog.tokens, o.index).lit_ptr, "open") || str_eq(VEC_GET(prog.tokens, o.index).lit_ptr, "close") || str_eq(VEC_GET(prog.tokens, o.index).lit_ptr, "malloc") || str_eq(VEC_GET(prog.tokens, o.index).lit_ptr, "free") || str_eq(VEC_GET(prog.tokens, o.index).lit_ptr, "read") || str_eq(VEC_GET(prog.tokens, o.index).lit_ptr, "exit") || str_eq(VEC_GET(prog.tokens, o.index).lit_ptr, "lseek"));
}

typedef struct {
    int index;
    long val;
    OpType type;
    size_t lit_ptr;
    int link;
} DefineData;
List(Defines, DefineData);

int find_previous_defined(Defines defines, size_t lit_ptr) {
    FOR_LIST(defines) {
        DefineData data = VEC_GET(defines, i);
        if (str_eq(data.lit_ptr, CSTR(lit_ptr))) {
            return i;
        }
    }
    return -1;
}

void delete_op(Program *prog, int index) {
    assert((index >= 0 && index < prog->cnt) && "Invalid index");
    int amount = (prog->cnt - index - 1) * sizeof(Op);
    void *dest = prog->data + index;    // data[index]
    void *src = prog->data + index + 1; // data[index++]
    memmove(dest, src, amount);
    prog->cnt--;
}

bool replace_defined(ProgramRun *prog) {
    int ip = 0;

    Defines defines = {0};

    while (VEC_GET(prog->vm.prog, ip).t != OP_NOP) {
        Op *it = &VEC_GET(prog->vm.prog, ip);
        if (it->t == OP_NOP || is_libc_word(*prog, *it))
            break;
        Op next = VEC_GET(prog->vm.prog, ip + 1);

        if (is_intrinsic(*it, W_DEF)) {
            // If is def or mem construct add to defines
            Op val = VEC_GET(prog->vm.prog, ip - 2);
            Op name = VEC_GET(prog->vm.prog, ip - 1);

            if (is_intrinsic(val, W_DEFINED)) {
                int idx = find_previous_defined(defines, VEC_GET(prog->tokens, val.index).lit_ptr);
                assert(idx >= 0 && "Something went to wrong");
                val.link = idx;
            }

            DefineData data = {
                defines.cnt,
                val.op,
                val.t,
                VEC_GET(prog->tokens, name.index).lit_ptr,
                val.link,
            };
            VEC_ADD(&defines, data);
        } else if (is_intrinsic(*it, W_MEM)) {
            Op val = VEC_GET(prog->vm.prog, ip - 2);
            Op *name = &VEC_GET(prog->vm.prog, ip - 1);

            if (is_intrinsic(val, W_DEFINED)) {
                int idx = find_previous_defined(defines, VEC_GET(prog->tokens, val.index).lit_ptr);
                if (idx == -1) {
                    assert(false);
                }
                DefineData data = VEC_GET(defines, idx);
                while (data.link != 0) {
                    data = VEC_GET(defines, data.link);
                }

                val.op = prog->vm.mem_ptr;
                prog->vm.mem_ptr += data.val;
                val.t = data.type;
            }

            name->link = defines.cnt;
            DefineData data = {
                defines.cnt,
                val.op,
                val.t,
                VEC_GET(prog->tokens, name->index).lit_ptr,
                val.link,
            };
            VEC_ADD(&defines, data);
        } else if (is_intrinsic(*it, W_DEFINED) && (!is_intrinsic(next, W_DEF) && !is_intrinsic(next, W_MEM))) {
            // If is defined or and not pre def or mem, find repr in defines and link
            int idx = find_previous_defined(defines, VEC_GET(prog->tokens, it->index).lit_ptr);
            if (idx < 0) {
                printf("Error: Word not defined %s\n", TOKEN_LIT(*prog, it->index));
                VEC_FREE(defines);
                return false;
            }
            it->link = idx;
        }
        ip += 1;
    }

    // Remove constant definitions and memory definitions
    // value name 'def' | value name 'mem'
    for (int i = prog->vm.prog.cnt - 1; i > 0; i--) {
        Op it = VEC_GET(prog->vm.prog, i);
        if (is_intrinsic(it, W_DEF) || is_intrinsic(it, W_MEM)) {
            delete_op(&prog->vm.prog, i);
            delete_op(&prog->vm.prog, i - 1);
            delete_op(&prog->vm.prog, i - 2);
        }
    }

    ip = 0;
    while (VEC_GET(prog->vm.prog, ip).t != OP_NOP) {
        Op *it = &VEC_GET(prog->vm.prog, ip);
        if (it->t == OP_NOP)
            break;
        if (is_intrinsic(*it, W_DEFINED) && !is_libc_word(*prog, *it)) {
            int idx = find_previous_defined(defines, VEC_GET(prog->tokens, it->index).lit_ptr);
            DefineData data = VEC_GET(defines, idx);
            if (data.type == OP_INTRINSIC && data.val == W_DEFINED) {
                it->op = defines.data[data.link].val;
                it->t = defines.data[data.link].type;
            } else {
                it->op = data.val;
                it->t = data.type;
            }
        }
        ip += 1;
    }

#ifdef DEBUG
    printf("=> Collected defines:\n");
    FOR_LIST(defines) {
        DefineData data = VEC_GET(defines, i);
        printf("    > [%d] %ld %s %s\n", i, data.val, op_to_str((Op){.t = data.type}), CSTR(data.lit_ptr));
    }
#endif

    VEC_FREE(defines);
    return true;
}

// ;defined

#define MAX_STACK 100

typedef enum {
    ERR_OVERFLOW,
    ERR_UNDERFLOW,
    ERR_UNCLOSED_LOOP,
    ERR_UNCLOSED_IF,
    ERR_NO_DO,
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

// :linker

void control_flow_link(VM *vm) {
    int ip = 0;

    while (vm->prog.data[ip].t != OP_NOP) {
        Op op = vm->prog.data[ip];
        if (op.t != OP_INTRINSIC) {
            ip++;
            continue;
        }

        if (is_intrinsic(op, W_LOOP)) {
            int loopIp = ip;
            int doIp = -1;
            int end = ip + 1;

            int loopCnt = 1;
            while (loopCnt > 0) {
                Op endOP = vm->prog.data[end];
                if (endOP.t == OP_NOP)
                    error(ERR_UNCLOSED_LOOP, endOP, "`do` requires an `end` keyword.\n");

                if (is_intrinsic(endOP, W_DO) && doIp == -1)
                    doIp = end;

                if (is_intrinsic(endOP, W_LOOP))
                    loopCnt++;
                if (is_intrinsic(endOP, W_END)) {
                    loopCnt--;
                    if (loopCnt == 0)
                        break;
                }
                end++;
            }

            if (doIp == -1)
                error(ERR_NO_DO, vm->prog.data[loopIp], "`do` keyword not found.\n");

            vm->prog.data[doIp].link = end + 1;
            vm->prog.data[end].link = loopIp;

        } else if (is_intrinsic(op, W_IF)) {
            int ifIp = ip;
            int end = ip + 1;
            int elseIp = -1;

            int ifCnt = 1;
            while (ifCnt > 0) {
                Op endOP = vm->prog.data[end];
                if (endOP.t == OP_NOP)
                    error(ERR_UNCLOSED_IF, endOP, "`if` requires and `endif` keyword\n");

                if (is_intrinsic(endOP, W_IF))
                    ifCnt++;
                if (is_intrinsic(endOP, W_ELSE) && ifCnt == 1)
                    elseIp = end;
                if (is_intrinsic(endOP, W_ENDIF)) {
                    ifCnt--;
                    if (ifCnt == 0)
                        break;
                }
                end++;
            }

            if (elseIp != -1) {
                vm->prog.data[ifIp].link = elseIp + 1;
                vm->prog.data[elseIp].link = end + 1;
            } else {
                vm->prog.data[ifIp].link = end + 1;
            }
        }
        ip++;
    }
}

// ;linker

// :interpeter
char *op_to_syntax(Op op) {
    static_assert(BT_COUNT == 8, "Implement newly added BinopType");
    static_assert(W_COUNT == 18, "Implement newly added IntrinsicType");
    static_assert(OP_COUNT == 13, "Implement newly added OpType");
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
    case OP_INTRINSIC: {
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
        case W_INCLUDE:
            return "include";
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

void can_pop_amount(int sp, long amt, Op o) {
    if (sp - amt < 0)
        error(ERR_UNDERFLOW, o, "`%s` requires at least %d value(s) on the stack.\n", op_to_syntax(o), amt);
}

long pop(long *stack, int *sp) {
    return stack[--(*sp)];
}

void try_push(long *stack, int *sp, long operand, Op o) {
    if (*sp + 1 >= MAX_STACK)
        error(ERR_OVERFLOW, o, "Can't push literal number %d\n", operand);
    stack[(*sp)++] = operand;
}

void interpet_binop(long *stack, int *sp, Op o) {
    static_assert(BT_COUNT == 8, "Implement newly added BinopType");

    can_pop_amount(*sp, 2, o);

    int top = pop(stack, sp);
    int ut = pop(stack, sp);

    switch (o.op) {
    case BT_PLUS:
        try_push(stack, sp, top + ut, o);
        break;
    case BT_MINUS:
        try_push(stack, sp, top - ut, o);
        break;
    case BT_MULT:
        try_push(stack, sp, top * ut, o);
        break;
    case BT_DIV:
        try_push(stack, sp, top / ut, o);
        break;
    case BT_MOD:
        try_push(stack, sp, top % ut, o);
        break;
    case BT_LT:
        try_push(stack, sp, top < ut, o);
        break;
    case BT_GT:
        try_push(stack, sp, top > ut, o);
        break;
    case BT_EQ:
        try_push(stack, sp, top == ut, o);
        break;
    }
}

void interpet_libc_call(long *stack, int *sp, int *ip, Op o, ProgramRun *prog) {
    _ ip;

    size_t to_check = VEC_GET(prog->tokens, o.index).lit_ptr;
    if (str_eq(to_check, "open")) {
        long path_id = pop(stack, sp);
        long mode = pop(stack, sp);

        char *path = prog->vm.mem + path_id;
        int fd = open(path, mode);
        try_push(stack, sp, fd, o);
    } else if (str_eq(to_check, "close")) {
        long fd = pop(stack, sp);

        close(fd);
    } else if (str_eq(to_check, "lseek")) {
        long fd = pop(stack, sp);
        long off = pop(stack, sp);
        long whence = pop(stack, sp);

        off_t i = lseek(fd, off, whence);

        try_push(stack, sp, i, o);
    } else if (str_eq(to_check, "malloc")) {
        int size = pop(stack, sp);
        long addr = (long)malloc(size);
        try_push(stack, sp, addr, o);
    } else if (str_eq(to_check, "read")) {
        long fd = pop(stack, sp);
        long addr = pop(stack, sp);
        long size = pop(stack, sp);

        read(fd, (void *)addr, size);
    } else if (str_eq(to_check, "free")) {
        long ptr = pop(stack, sp);
        free((void *)ptr);
    } else if (str_eq(to_check, "exit")) {
        int code = pop(stack, sp);
        exit(code);
    }
}

void interpet_intrinsic(long *stack, int *sp, int *ip, Op o, ProgramRun *prog) {
    switch (o.op) {
    case W_DEFINED: {
        if (is_libc_word(*prog, o)) {
            interpet_libc_call(stack, sp, ip, o, prog);
        } else {
            int def_id = find_defined(*prog, o);
            if (def_id == -1) {
                printf("Something went wrong!\n");
            }
            if (prog->vm.constData[def_id] != -1) {
                try_push(stack, sp, prog->vm.constData[def_id], o);
            } else {
                try_push(stack, sp, def_id, o);
            }
        }
        *ip += 1;
    } break;
    case W_PUTD: {
        can_pop_amount(*sp, 1, o);
        long top = pop(stack, sp);
        printf("%ld", top);
        *ip += 1;
    } break;
    case W_LOOP: {
        *ip += 1;
    } break;
    case W_END: {
        *ip = o.link;
    } break;
    case W_DO: {
        can_pop_amount(*sp, 1, o);
        long top = pop(stack, sp);
        if (top)
            *ip += 1;
        else
            *ip = o.link;
    } break;
    case W_PUTC: {
        can_pop_amount(*sp, 1, o);
        int top = pop(stack, sp);
        printf("%c", top);
        *ip += 1;
    } break;
    case W_PRINTLN: {
        can_pop_amount(*sp, 1, o);
        long top = pop(stack, sp);
        char *str = prog->vm.mem + top;
        printf("%s\n", str);
        *ip += 1;
    } break;
    case W_PRINT: {
        can_pop_amount(*sp, 1, o);
        long top = pop(stack, sp);
        char *str = prog->vm.mem + top;
        printf("%s", str);
        *ip += 1;
    } break;
    case W_IF: {
        can_pop_amount(*sp, 1, o);
        long top = pop(stack, sp);
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
    case W_MEM: {
        assert(false && "unreachable");
    } break;
    case W_W_MEM: {
        can_pop_amount(*sp, 2, o);
        long defined_id = pop(stack, sp);
        long val = pop(stack, sp);
        long off = prog->vm.definedData[defined_id];

        memcpy(prog->vm.mem + off, &val, sizeof(long));

        *ip += 1;
    } break;
    case W_W_MEM64: {
        can_pop_amount(*sp, 2, o);
        long ptr = pop(stack, sp);
        long val = pop(stack, sp);

        memcpy(prog->vm.mem + ptr, &val, sizeof(long));

        *ip += 1;
    } break;

    case W_DEREF: {
        can_pop_amount(*sp, 2, o);
        long ptr = pop(stack, sp);
        long size = pop(stack, sp);

        long at;
        memcpy(&at, prog->vm.mem + ptr, size);
        try_push(stack, sp, at, o);

        *ip += 1;
    } break;
    case W_AS_STR: {
        can_pop_amount(*sp, 2, o);
        long ptr = pop(stack, sp);
        long size = pop(stack, sp);

        size_t temp_ptr = cstr_cpy((void *)ptr, size);
        try_push(stack, sp, push_str_to_mem(&prog->vm, temp_ptr), o);

        *ip += 1;
    } break;
    case W_DEF: {
        assert(false && "unreachable");
    } break;
    case W_INCLUDE: {
        assert(false && "not implemented yet");
    } break;
    default:
        printf("Word not handled %s %s\n", op_to_str(o), TOKEN_LIT(*prog, o.index));
        exit(1);
    }
}

bool interpet(ProgramRun *prog) {
    long stack[MAX_STACK] = {0};
    int sp = 0;
    long backStack[MAX_STACK] = {0};
    int bsp = 0;

    VM vm = prog->vm;

    static_assert(OP_COUNT == 13, "Implement newly added OpType");
    int ip = 0;
    while (VEC_GET(vm.prog, ip).t != OP_NOP) {
        Op o = VEC_GET(vm.prog, ip);
        switch (o.t) {
        case OP_BINOP: {
            interpet_binop(stack, &sp, o);
            ip++;
        } break;
        case OP_LIT_NUMBER:
        case OP_LIT_STR: {
            try_push(stack, &sp, o.op, o);
            ip++;
        } break;
        case OP_INTRINSIC: {
            interpet_intrinsic(stack, &sp, &ip, o, prog);
        } break;
        case OP_DUMP: {
            printf("> Stack Dump:\n");
            for (int j = 0; j < sp; j++) {
                printf("[%d] %ld\n", j, stack[j]);
            }
            printf("< End Stack Dump.\n");
            ip++;
        } break;
        case OP_BDUMP: {
            printf("> Back Stack Dump:\n");
            for (int j = 0; j < sp; j++) {
                printf("[%d] %ld\n", j, backStack[j]);
            }
            printf("< End Back Stack Dump.\n");
            ip++;
        } break;
        case OP_DUP: {
            can_pop_amount(sp, 1, o);
            try_push(stack, &sp, stack[sp - 1], o);
            ip++;
        } break;
        case OP_2DUP: {
            can_pop_amount(sp, 2, o);
            long top = stack[sp - 1];
            long ut = stack[sp - 2];
            try_push(stack, &sp, ut, o);
            try_push(stack, &sp, top, o);
            ip++;
        } break;
        case OP_DROP: {
            can_pop_amount(sp, 1, o);
            pop(stack, &sp);
            ip++;
        } break;
        case OP_SWAP: {
            can_pop_amount(sp, 2, o);
            long top = pop(stack, &sp);
            long ut = pop(stack, &sp);
            try_push(stack, &sp, top, o);
            try_push(stack, &sp, ut, o);
            ip++;
        } break;
        case OP_STASH: {
            can_pop_amount(sp, 1, o);
            long top = pop(stack, &sp);
            if (sp - top < 0)
                error(ERR_UNDERFLOW, o, "Trying to stash %d values on the stack, but only %d are available.\n", top, sp);
            if (bsp + top >= MAX_STACK)
                error(ERR_OVERFLOW, o, "Can't stash %d values on back stack.\n", top);

            for (int n = 0; n < top; n++)
                backStack[bsp++] = stack[--sp];
            ip++;
        } break;
        case OP_POP: {
            can_pop_amount(sp, 1, o);
            long top = pop(stack, &sp);
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
            printf("[%d] %ld\n", i, stack[i]);
        }
    }

    return true;
}

// ;interpet

void clean_program_run(ProgramRun *prog) {
    VEC_FREE(prog->vm.definedTable);
    VEC_FREE(prog->vm.prog);
    VEC_FREE(prog->tokens);

    free(prog->code);
}

ProgramRun run_program(const char *path) {
    size_t len;
    char *code = read_file_as_cstr(path, &len);
    if (code == NULL)
        exit(1);

    ProgramRun res = {0};
    res.code = code;
    bench b = {0};
    BENCH_START(&b);
    tokenize(code, len, path, &res.tokens);
    MEASURE(&b, "Tokenize");

    memset(res.vm.constData, -1, sizeof(int) * MAX_DEFINED);
    parse(&res);
    MEASURE(&b, "Parse tokens");

#ifdef DEBUG
    // print_operations(vm);
#endif
    BENCH_START(&b);
    replace_defined(&res);

    MEASURE(&b, "Constant fold");
    BENCH_START(&b);
    control_flow_link(&res.vm);
    MEASURE(&b, "ControlFlowLink");

#ifdef DEBUG
    print_operations(res);
#endif

    BENCH_START(&b);
    interpet(&res);
    MEASURE(&b, "Interpet");

    return res;
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

    ProgramRun run = run_program(path);
    clean_program_run(&run);

    return 0;
}
