// GUEYE Sokhna Faty 260ADB182
// https://github.com/fatyy1902/Lab_1_graded
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Chat gpt, conversation history: https://chatgpt.com/share/6adf8bf9-c934-83eb-b22d-9cb6dab864d1; course slides DE0917 (Arrays, Strings & Pointers; Files, Arguments, Directories; Advanced pointers, dynamic memory, structures, preprocessor directives)

// The tokenizer part (skip() and the reading of numbers and   operators) was written by Chqt gpt. I read it and I can explain it.

#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define NAME "Sokhna"
#define LASTNAME "GUEYE"
#define ID "260ADB182"

enum Result { RES_OK = 0, RES_FAIL = 1 };

// Course: enum. Each kind of token has a name, so the parser compares kinds instead of characters.
typedef enum {
    T_NUM,   // a number
    T_PLUS,  // +
    T_MINUS, // -
    T_MUL,   // *
    T_DIV,   // /
    T_POW,   // **
    T_LP,    // (
    T_RP,    // )
    T_HASH,  // #  (a comment if it is the first token of the line, an error anywhere else)
    T_END,   // end of the line ('\n') or of the file
    T_BAD    // any other character
} TokenKind;

// Integer if both operands are integers, otherwise both become double (7 / 2 is 3, 7 / 2.0 is 3.5).
typedef struct {
    int is_float;
    long long i;
    double d;
} Value;

// start points to the first character of the token in the file. Errors are reported at start
typedef struct {
    TokenKind kind;
    char *start;
    Value val; 
} Token;

// Positions: the whole file is one string and p walks along it, so position = (start - text) + 1.
// Comments, blank lines and every '\n' are in that string, so they are counted automatically.
// tok is the token the parser is looking at (one token of lookahead).
typedef struct {
    char *text;
    char *p;
    Token tok;
    size_t err; // position of the first error, 0 = no error
} Parser;

static Value int_value(long long i) {
    Value v = {0, i, 0.0};
    return v;
}

static Value float_value(double d) {
    Value v = {1, 0, d};
    return v;
}

static double as_double(Value v) {
    return v.is_float ? v.d : (double)v.i;
}

// Only the first error is kept.
static void fail_at(Parser *ps, const char *where) {
    if (!ps->err) ps->err = (size_t)(where - ps->text) + 1;
}

// Tokenizer (AI): reads the next token into ps->tok. It never reports an error itself: a wrong
// character becomes T_BAD, and the parser reports it when it cannot accept it. This way errors stay
// in left-to-right order (in "10 / 0 &" the division by zero comes before the '&').
// '\n' is not skipped: it is T_END, so a missing operand or a missing ')' is reported on it.
static void next_token(Parser *ps) {
    while (*ps->p == ' ' || *ps->p == '\t') ps->p++;
    Token *t = &ps->tok;
    t->start = ps->p;
    t->val = int_value(0);
    char c = *ps->p;

    if (c == '\0' || c == '\n') {
        t->kind = T_END;
        return;
    }
    if (isdigit((unsigned char)c)) {
        unsigned long long u = 0; // unsigned: overflow is defined, signed overflow is not
        while (isdigit((unsigned char)*ps->p)) u = u * 10 + (unsigned long long)(*ps->p++ - '0');
        t->val = int_value((long long)u);
        // A float is digits, a '.', then digits. So "5." and ".5" stop before the '.', which is then
        // a T_BAD token (error at the '.'), and "1.5e3" stops before the 'e'.
        if (ps->p[0] == '.' && isdigit((unsigned char)ps->p[1])) {
            ps->p++;
            while (isdigit((unsigned char)*ps->p)) ps->p++;
            char saved = *ps->p; // strtod needs a '\0' after the number: put one there, no copy
            *ps->p = '\0';
            t->val = float_value(strtod(t->start, NULL));
            *ps->p = saved;
        }
        t->kind = T_NUM;
        return;
    }

    ps->p++;
    switch (c) {
    case '+': t->kind = T_PLUS; break;
    case '-': t->kind = T_MINUS; break;
    case '/': t->kind = T_DIV; break;
    case '(': t->kind = T_LP; break;
    case ')': t->kind = T_RP; break;
    case '#': t->kind = T_HASH; break;
    case '*':
        // "**" is one token, so "2 ** 3" is never read as "2 * * 3".
        if (*ps->p == '*') {
            ps->p++;
            t->kind = T_POW;
        } else {
            t->kind = T_MUL;
        }
        break;
    default: t->kind = T_BAD; break;
    }
}

// op is T_PLUS, T_MINUS, T_MUL, T_DIV or T_POW. Division by zero and a negative integer exponent are
// reported at the right operand (the wrong part). A float result that is infinite or not a number
// is reported at the operator, because no single operand is wrong.
static Value arith(Parser *ps, TokenKind op, Value a, Value b, const char *op_at, const char *rhs_at) {
    if (!a.is_float && !b.is_float) {
        switch (op) {
        case T_PLUS: return int_value(a.i + b.i);
        case T_MINUS: return int_value(a.i - b.i);
        case T_MUL: return int_value(a.i * b.i);
        case T_DIV:
            if (b.i == 0) {
                fail_at(ps, rhs_at);
                return int_value(0);
            }
            return int_value(a.i / b.i);
        default: {
            if (b.i < 0) {
                fail_at(ps, rhs_at);
                return int_value(0);
            }
            unsigned long long r = 1, x = (unsigned long long)a.i; // unsigned: no overflow problem
            for (long long e = b.i; e > 0; e >>= 1) {
                if (e & 1) r *= x;
                x *= x;
            }
            return int_value((long long)r);
        }
        }
    }

    double x = as_double(a), y = as_double(b), r;
    if (op == T_PLUS) r = x + y;
    else if (op == T_MINUS) r = x - y;
    else if (op == T_MUL) r = x * y;
    else if (op == T_DIV) {
        if (y == 0.0) {
            fail_at(ps, rhs_at);
            return float_value(0.0);
        }
        r = x / y;
    } else {
        r = pow(x, y);
    }
    if (!isfinite(r)) {
        fail_at(ps, op_at);
        return float_value(0.0);
    }
    return float_value(r);
}

// Precedence: expr is made of terms and a term is made of powers,
// so ** is done first, then * and /, then + and -.
//   expr    := term  { ('+' | '-') term }
//   term    := power { ('*' | '/') power }
//   power   := primary [ '**' power ]
//   primary := NUMBER | '(' expr ')'
static Value expr(Parser *ps);

// An operand is expected here, so every "missing operand" error (second operator, "()", end of line, bad character) is reported at the token we are looking at. 
static Value primary(Parser *ps) {
    if (ps->tok.kind == T_NUM) {
        Value v = ps->tok.val;
        next_token(ps);
        return v;
    }
    if (ps->tok.kind == T_LP) {
        next_token(ps);
        Value v = expr(ps);
        if (ps->err) return v;
        if (ps->tok.kind != T_RP) { 
            fail_at(ps, ps->tok.start);
            return v;
        }
        next_token(ps);
        return v;
    }
    fail_at(ps, ps->tok.start);
    return int_value(0);
}

static Value power(Parser *ps) {
    Value a = primary(ps);
    if (ps->err || ps->tok.kind != T_POW) return a;
    const char *op_at = ps->tok.start;
    next_token(ps);
    const char *rhs_at = ps->tok.start;
    Value b = power(ps);
    if (ps->err) return int_value(0);
    return arith(ps, T_POW, a, b, op_at, rhs_at);
}

static Value term(Parser *ps) {
    Value a = power(ps);
    while (!ps->err && (ps->tok.kind == T_MUL || ps->tok.kind == T_DIV)) {
        TokenKind op = ps->tok.kind;
        const char *op_at = ps->tok.start;
        next_token(ps);
        const char *rhs_at = ps->tok.start;
        Value b = power(ps);
        if (ps->err) return int_value(0);
        a = arith(ps, op, a, b, op_at, rhs_at);
    }
    return a;
}

static Value expr(Parser *ps) {
    Value a = term(ps);
    while (!ps->err && (ps->tok.kind == T_PLUS || ps->tok.kind == T_MINUS)) {
        TokenKind op = ps->tok.kind;
        const char *op_at = ps->tok.start;
        next_token(ps);
        const char *rhs_at = ps->tok.start;
        Value b = term(ps);
        if (ps->err) return int_value(0);
        a = arith(ps, op, a, b, op_at, rhs_at);
    }
    return a;
}

static int make_folder(const char *folder) {
    struct stat st;
    if (stat(folder, &st) == 0) return RES_OK;
    if (mkdir(folder, 0700) != 0) {
        perror(folder);
        return RES_FAIL;
    }
    return RES_OK;
}

static int process(const char *inpath, const char *base, const char *outdir) {
    FILE *f = fopen(inpath, "rb");
    if (f == NULL) {
        perror(inpath);
        return RES_FAIL;
    }
    struct stat st;
    if (stat(inpath, &st) != 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "calc: %s is not a regular file\n", inpath);
        fclose(f);
        return RES_FAIL;
    }

    // the file is read once into one buffer and parsed there, with no copy per line.
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buf = (size < 0) ? NULL : malloc((size_t)size + 1);
    if (buf == NULL) {
        fprintf(stderr, "calc: cannot read %s\n", inpath);
        fclose(f);
        return RES_FAIL;
    }
    size_t n = fread(buf, 1, (size_t)size, f);
    int read_failed = ferror(f);
    fclose(f);
    if (read_failed) {
        fprintf(stderr, "calc: error reading %s\n", inpath);
        free(buf);
        return RES_FAIL;
    }
    buf[n] = '\0';

    // after an error only the ERROR line must be written.
    Value *res = NULL;
    size_t count = 0, cap = 0;
    Parser ps;
    ps.text = buf;
    ps.p = buf;
    ps.err = 0;
    ps.tok.kind = T_END;
    ps.tok.start = buf;
    ps.tok.val = int_value(0);
    while (*ps.p && !ps.err) {
        next_token(&ps); // first token of the line
        if (ps.tok.kind == T_END) { // blank line: no output
            if (*ps.p == '\n') ps.p++;
            continue;
        }
        if (ps.tok.kind == T_HASH) { // '#' as the FIRST token of the line: a comment
            while (*ps.p && *ps.p != '\n') ps.p++;
            if (*ps.p == '\n') ps.p++;
            continue;
        }
        Value v = expr(&ps);
        if (!ps.err && ps.tok.kind != T_END) fail_at(&ps, ps.tok.start); // leftover: extra ')', '#', "2 3"
        if (ps.err) break;
        if (count == cap) {
            size_t new_cap = cap ? cap * 2 : 1024;
            Value *temp = realloc(res, new_cap * sizeof *res);
            if (temp == NULL) {
                fprintf(stderr, "calc: out of memory\n");
                free(res);
                free(buf);
                return RES_FAIL;
            }
            res = temp;
            cap = new_cap;
        }
        res[count++] = v;
        if (*ps.p == '\n') ps.p++;
    }

    int rc = RES_FAIL;
    char path[1024];
    snprintf(path, sizeof path, "%s/%s_%s_%s_%s.txt", outdir, base, NAME, LASTNAME, ID);
    if (make_folder(outdir) == RES_OK) {
        FILE *o = fopen(path, "w");
        if (o == NULL) {
            perror(path);
        } else {
            if (ps.err) {
                fprintf(o, "ERROR:%zu\n", ps.err);
            } else {
                for (size_t i = 0; i < count; i++) {
                    if (res[i].is_float) fprintf(o, "%.15g\n", res[i].d);
                    else fprintf(o, "%lld\n", res[i].i);
                }
            }
            if (fclose(o) != 0) fprintf(stderr, "calc: error writing %s\n", path);
            else rc = RES_OK;
        }
    }
    free(res);
    free(buf);
    return rc;
}

// Last part of the path, without ".txt" and without a final '/'.
static void base_name(const char *path, char *out, size_t size) {
    char tmp[1024];
    snprintf(tmp, sizeof tmp, "%s", path);
    size_t n = strlen(tmp);
    while (n > 1 && tmp[n - 1] == '/') tmp[--n] = '\0';
    const char *s = strrchr(tmp, '/');
    snprintf(out, size, "%s", s ? s + 1 : tmp);
    n = strlen(out);
    if (n > 4 && strcmp(out + n - 4, ".txt") == 0) out[n - 4] = '\0';
}

static int usage(void) {
    fprintf(stderr, "usage: calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n");
    return RES_FAIL;
}

int main(int argc, char *argv[]) {
    const char *dir = NULL, *outdir = NULL, *input = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "calc: no folder after %s\n", argv[i]);
                return usage();
            }
            dir = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "calc: no folder after %s\n", argv[i]);
                return usage();
            }
            outdir = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "calc: unknown option %s\n", argv[i]);
            return usage();
        } else {
            input = argv[i];
        }
    }
    if ((dir == NULL) == (input == NULL)) return usage(); // exactly one of: input file, -d DIR

    char base[256], default_out[1024];
    base_name(dir ? dir : input, base, sizeof base);
    if (outdir == NULL) {
        snprintf(default_out, sizeof default_out, "%s_%s_%s", base, NAME, ID);
        outdir = default_out;
    }

    if (input) return process(input, base, outdir);

    DIR *d = opendir(dir);
    if (d == NULL) {
        perror(dir);
        return RES_FAIL;
    }
    int rc = RES_OK;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        size_t n = strlen(entry->d_name);
        if (n <= 4 || strcmp(entry->d_name + n - 4, ".txt") != 0) continue;
        char full[1024];
        snprintf(full, sizeof full, "%s/%s", dir, entry->d_name);
        struct stat st;
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue; // ignore subfolders
        char file_base[1024];
        base_name(full, file_base, sizeof file_base);
        if (process(full, file_base, outdir) != RES_OK) rc = RES_FAIL;
    }
    closedir(d);
    return rc;
}