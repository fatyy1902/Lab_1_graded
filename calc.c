// GUEYE Sokhna Faty 260ADB182
// (optional) https://github.com/<your-username>/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <every AI tool, website and book you used; details in comments where you used them>

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


#define NAME "Sokhna"
#define LASTNAME "GUEYE"
#define ID "260ADB182"
#define PATH_SIZE 1024 // size of every path buffer (course, "Always reserve enough space in buffers")


enum Result { RES_OK = 0, RES_FAIL = 1 };


typedef struct {
    const char *text; 
    const char *p;    
    size_t err;      
} Parser;

// Only the FIRST error counts, hence the `if (!ps->err)`.
static void fail_at(Parser *ps, const char *where) {
    if (!ps->err) ps->err = (size_t)(where - ps->text) + 1;
}

// Skip space between token
static void skip(Parser *ps) {
    while (*ps->p == ' ' || *ps->p == '\t') ps->p++;
}

// Recursive descent, one function per precedence level:
//   expr    := term  { ('+' | '-') term }
//   term    := power { ('*' | '/') power }
//   power   := primary [ '**' power ]
//   primary := NUMBER | '(' expr ')'
static long long expr(Parser *ps);

static long long primary(Parser *ps) {
    skip(ps);
    if (isdigit((unsigned char)*ps->p)) {
        long long v = 0;
        while (isdigit((unsigned char)*ps->p)) v = v * 10 + (*ps->p++ - '0');
        return v;
    }
    if (*ps->p == '(') {
        ps->p++;
        long long v = expr(ps);
        skip(ps);
        if (!ps->err && *ps->p != ')') fail_at(ps, ps->p); 
        if (!ps->err) ps->p++;
        return v;
    }
    fail_at(ps, ps->p); 
    return 0;
}

static long long power(Parser *ps) {
    long long a = primary(ps);
    skip(ps);
    if (ps->err || ps->p[0] != '*' || ps->p[1] != '*') return a;
    ps->p += 2;
    skip(ps);
    const char *at = ps->p; 
   
    long long b = power(ps);
    if (ps->err) return 0;
    if (b < 0) {
        fail_at(ps, at);
        return 0;
    }
   
    unsigned long long r = 1, x = (unsigned long long)a;
    for (; b > 0; b >>= 1) {
        if (b & 1) r *= x;
        x *= x;
    }
    return (long long)r;
}


static long long term(Parser *ps) {
    long long a = power(ps);
    for (;;) {
        skip(ps);
        if (ps->err || (*ps->p != '*' && *ps->p != '/')) return a;
        char op = *ps->p++;
        skip(ps);
        const char *at = ps->p;
        long long b = power(ps);
        if (ps->err) return 0;
        if (op == '*') {
            a *= b;
        } else if (b == 0) { 
            fail_at(ps, at);
            return 0;
        } else {
            a /= b; 
        }
    }
}

static long long expr(Parser *ps) {
    long long a = term(ps);
    for (;;) {
        skip(ps);
        if (ps->err || (*ps->p != '+' && *ps->p != '-')) return a;
        char op = *ps->p++;
        long long b = term(ps);
        if (ps->err) return 0;
        a = (op == '+') ? a + b : a - b;
    }
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

    
    long long *res = NULL;
    size_t count = 0, cap = 0;
    Parser ps = {buf, buf, 0};
    while (*ps.p && !ps.err) {
        skip(&ps);
        if (*ps.p == '\n') { ps.p++; continue; }                             
        if (*ps.p == '#') { while (*ps.p && *ps.p != '\n') ps.p++; continue; } 
        if (*ps.p == '\0') break;                                            
        long long v = expr(&ps);
        skip(&ps);
        
        if (!ps.err && *ps.p != '\n' && *ps.p != '\0') fail_at(&ps, ps.p);
        if (ps.err) break;
        if (count == cap) {
            size_t new_cap = cap ? cap * 2 : 1024;
            long long *temp = realloc(res, new_cap * sizeof *res);
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
    char path[PATH_SIZE];
    snprintf(path, sizeof path, "%s/%s_%s_%s_%s.txt", outdir, base, NAME, LASTNAME, ID);
    if (make_folder(outdir) == RES_OK) {
        FILE *o = fopen(path, "w");
        if (o == NULL) {
            perror(path);
        } else {
            if (ps.err) fprintf(o, "ERROR:%zu\n", ps.err);
            else for (size_t i = 0; i < count; i++) fprintf(o, "%lld\n", res[i]);
            if (fclose(o) != 0) fprintf(stderr, "calc: error writing %s\n", path);
            else rc = RES_OK;
        }
    }
    free(res);
    free(buf);
    return rc;
}


static void base_name(const char *path, char *out, size_t size) {
    char tmp[PATH_SIZE];
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
    
    if ((dir == NULL) == (input == NULL)) return usage();

    char base[256], default_out[PATH_SIZE];
    base_name(dir ? dir : input, base, sizeof base);
    if (outdir == NULL) { // default output folder: <base>_<name>_<id>
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
        char full[PATH_SIZE];
        snprintf(full, sizeof full, "%s/%s", dir, entry->d_name);
        struct stat st;
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue; // subfolders are ignored
        char file_base[PATH_SIZE];
        base_name(full, file_base, sizeof file_base);
        if (process(full, file_base, outdir) != RES_OK) rc = RES_FAIL;
    }
    closedir(d);
    return rc;
}