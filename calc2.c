// GUEYE Sokhna Faty 260ADB182
// (optional) https://github.com/<your-username>/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <every AI tool, website and book you used; details in comments where you used them>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <math.h>

#define STUDENT_NAME "Sokhna"
#define STUDENT_LASTNAME "GUEYE"
#define STUDENT_ID "260ADB182"

enum TokenType
{
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

typedef struct
{
    enum TokenType type;
    long long int_val;
    double float_val;
    size_t pos;
} Token;

typedef struct
{
    int is_float;
    long long i_val;
    double f_val;
    size_t start_pos;
} Value;

typedef struct
{
    char *text;
    size_t length;
    size_t index;
    Token current;
    size_t err_pos;
} Parser;

Value parse_expr(Parser *p);

int is_digit(char c)
{
    return (c >= '0' && c <= '9');
}

int is_valid_float(double x)
{
    if (x != x)
    {
        return 0;
    }
    if (x > 1.7976931348623157e308 || x < -1.7976931348623157e308)
    {
        return 0;
    }
    return 1;
}

// AI (Gemini): prompt "write a tokenizer in C  that tracks 1-based character positions and parses integers and floats"

void next_token(Parser *p)
{
    // Skip spaces and tabs between tokens
    while (p->index < p->length)
    {
        char c = *(p->text + p->index);
        if (c == ' ' || c == '\t' || c == '\r')
        {
            p->index++;
        }
        else
        {
            break;
        }
    }

    if (p->index >= p->length)
    {
        p->current.type = TOK_EOL;
        p->current.pos = p->length + 1;
        return;
    }

    char c = p->text[p->index];
    size_t start_pos = p->index + 1;

    if (c == '\n')
    {
        p->current.type = TOK_EOL;
        p->current.pos = start_pos;
        return;
    }

    if (c == '+')
    {
        p->index++;
        p->current.type = TOK_PLUS;
        p->current.pos = start_pos;
        return;
    }
    if (c == '-')
    {
        p->index++;
        p->current.type = TOK_MINUS;
        p->current.pos = start_pos;
        return;
    }
    if (c == '/')
    {
        p->index++;
        p->current.type = TOK_DIV;
        p->current.pos = start_pos;
        return;
    }
    if (c == '(')
    {
        p->index++;
        p->current.type = TOK_LPAREN;
        p->current.pos = start_pos;
        return;
    }
    if (c == ')')
    {
        p->index++;
        p->current.type = TOK_RPAREN;
        p->current.pos = start_pos;
        return;
    }

    if (c == '*')
    {
        if (p->index + 1 < p->length && p->text[p->index + 1] == '*')
        {
            p->index += 2;
            p->current.type = TOK_POW;
            p->current.pos = start_pos;
        }
        else
        {
            p->index++;
            p->current.type = TOK_MUL;
            p->current.pos = start_pos;
        }
        return;
    }

    if (is_digit(c))
    {
        size_t num_start = p->index;
        while (p->index < p->length && is_digit(p->text[p->index]))
        {
            p->index++;
        }

        if (p->index < p->length && p->text[p->index] == '.')
        {
            size_t dot_pos = p->index + 1;
            p->index++; // skip '.'

            // Rule: '5.' without digits after '.' is an error at the '.'
            if (p->index >= p->length || !is_digit(p->text[p->index]))
            {
                p->current.type = TOK_INVALID;
                p->current.pos = dot_pos;
                return;
            }

            while (p->index < p->length && is_digit(p->text[p->index]))
            {
                p->index++;
            }

            // Copy only digits.digits into a temporary string so strtod
            // does not read 'e' in '1.5e3' (leaving 'e' to trigger an error at 'e')
            size_t len = p->index - num_start;
            char num_str[128];
            if (len >= sizeof(num_str))
            {
                len = sizeof(num_str) - 1;
            }
            for (size_t i = 0; i < len; i++)
            {
                num_str[i] = p->text[num_start + i];
            }
            num_str[len] = '\0';

            p->current.type = TOK_FLOAT;
            p->current.float_val = strtod(num_str, NULL);
            p->current.pos = start_pos;
            return;
        }
        else
        {
            long long val = 0;
            for (size_t i = num_start; i < p->index; i++)
            {
                val = val * 10 + (p->text[i] - '0');
            }
            p->current.type = TOK_INT;
            p->current.int_val = val;
            p->current.float_val = (double)val;
            p->current.pos = start_pos;
            return;
        }
    }

    // Any other character ('&', '#', '.', 'e', etc.) is an invalid character
    p->index++;
    p->current.type = TOK_INVALID;
    p->current.pos = start_pos;
}


void set_error(Parser *p, size_t pos) {
    if (p->err_pos == 0) {
        p->err_pos = pos;
    }
}





double to_double(Value v) {
    if (v.is_float) {
        return v.f_val;
    }
    return (double)v.i_val;
}



long long calc_int_pow(long long base, long long exp) {
    long long res = 1;
    while (exp > 0) {
        if (exp % 2 == 1) {
            res *= base;
        }
        base *= base;
        exp /= 2;
    }
    return res;
}



Value parse_primary(Parser *p) {
    Value v = {0, 0, 0.0, p->current.pos};
    if (p->err_pos != 0) return v;

    if (p->current.type == TOK_INT) {
        v.is_float = 0;
        v.i_val = p->current.int_val;
        v.start_pos = p->current.pos;
        next_token(p);
        return v;
    }

    if (p->current.type == TOK_FLOAT) {
        if (!isfinite(p->current.float_val)) {
            set_error(p, p->current.pos);
            return v;
        }
        v.is_float = 1;
        v.f_val = p->current.float_val;
        v.start_pos = p->current.pos;
        next_token(p);
        return v;
    }

    if (p->current.type == TOK_LPAREN) {
        size_t open_pos = p->current.pos;
        next_token(p); 

        Value inside = parse_expr(p);
        if (p->err_pos != 0) return inside;

        
        if (p->current.type != TOK_RPAREN) {
            set_error(p, p->current.pos);
            return inside;
        }
        next_token(p); 
        inside.start_pos = open_pos;
        return inside;
    }

   
    set_error(p, p->current.pos);
    return v;
}






