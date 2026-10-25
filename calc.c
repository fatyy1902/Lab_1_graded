// GUEYE Sokhna Faty 260ADB182
// (optional) https://github.com/<your-username>/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <every AI tool, website and book you used; details in comments where you used them>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>


#define STUDENT_NAME "Sokhna"
#define STUDENT_LASTNAME "GUEYE"
#define STUDENT_ID "260ADB182"


enum TokenType {
    TOK_INT,
    TOK_FLOAT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_POW,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EOL,
    TOK_INVALID
};

typedef struct {
    enum TokenType type;
    long long int_val;
    double float_val;
    size_t pos;
} Token;

typedef struct {
    int is_float;
    long long i_val;
    double f_val;
    size_t start_pos;
} Value;

