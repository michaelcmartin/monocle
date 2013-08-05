#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "json.h"

typedef struct {
    TREE_NODE header;
    JSON_VALUE *value;
    char key[0];
} JSON_OBJECT_NODE;

/* JSON Parse context. */
typedef struct {
    const char *s; /* The string containing the JSON to parse. NOT
                    * necessarily null-terminated! */
    size_t size;   /* The length of s */
    int i;         /* The parser's "cursor" location, as an offset */
    int line, col; /* The parser's "cursor" location, in the file */
} JSON_PARSE_CTX;

/* Strings and arrays need an extra dummy field to store the extra data. */

typedef struct {
    JSON_VALUE core;
    char str[0];
} JSON_STRING_VALUE;

typedef struct {
    JSON_VALUE core;
    JSON_VALUE *array[0];
} JSON_ARRAY_VALUE;

static char error_str[512] = "";
JSON_VALUE json_dummy;
JSON_VALUE *json_ok = &json_dummy;

const char *
json_error()
{
    return error_str;
}

static void
free_object_node(TREE_NODE *node) {
    JSON_OBJECT_NODE *n = (JSON_OBJECT_NODE *)node;
    json_free(n->value);
    free(n);
}

void
json_free (JSON_VALUE *json)
{
    int i;
    if (!json) {
        return;
    }
    switch (json->tag) {
    case JSON_ARRAY:
        for (i = 0; i < json->value.array.size; ++i) {
            json_free(json->value.array.data[i]);
        }
        break;
    case JSON_OBJECT:
        tree_postorder(&json->value.object, free_object_node);
        break;
    default:
        /* No other types have subvalues */
        break;
    }
    free (json);
}

/* We have this return an int because the ctype functions take ints
 * and that can get ugly. */
static int
peekch (JSON_PARSE_CTX *ctx)
{
    if (ctx->i >= ctx->size) {
        return '\0';
    }
    return (int)((unsigned char)(ctx->s[ctx->i]));
}

static int
readch (JSON_PARSE_CTX *ctx)
{
    int c = peekch(ctx);
    if (c) {
        ++ctx->i;
        ++ctx->col;
        if (c == '\n') {
            ctx->col = 1;
            ++ctx->line;
        }
    }
    return c;
}

/* Skip whitespace */
static void
space (JSON_PARSE_CTX *ctx)
{
    while (1) {
        int c = peekch(ctx);
        if (!c || !isspace(c)) {
            break;
        }
        readch(ctx);
    }
}    

static JSON_VALUE *
word(JSON_PARSE_CTX *ctx, int scan)
{
    const char *s = ctx->s + ctx->i;
    if (*s == 'n' && ctx->i + 4 <= ctx->size && !strncmp(s, "null", 4)) {
        int i;
        JSON_VALUE *result;
        for (i = 0; i < 4; ++i) {
            readch(ctx);
        }
        if (scan) {
            return json_ok;
        }
        result = (JSON_VALUE *)malloc(sizeof(JSON_VALUE));
        if (!result) {
            snprintf(error_str, 512, "%d:%d: Out of memory", ctx->line, ctx->col);
            return NULL;
        }
        result->tag = JSON_NULL;
        return result;
    } else if (*s == 't' && ctx->i + 4 <= ctx->size && !strncmp(s, "true", 4)) {
        int i;
        JSON_VALUE *result;
        for (i = 0; i < 4; ++i) {
            readch(ctx);
        }
        if (scan) {
            return json_ok;
        }
        result = (JSON_VALUE *)malloc(sizeof(JSON_VALUE));
        if (!result) {
            snprintf(error_str, 512, "%d:%d: Out of memory", ctx->line, ctx->col);
            return NULL;
        }
        result->tag = JSON_BOOLEAN;
        result->value.boolean = 1;
        return result;
    } else if (*s == 'f' && ctx->i + 5 <= ctx->size && !strncmp(s, "false", 5)) {
        int i;
        JSON_VALUE *result;
        for (i = 0; i < 5; ++i) {
            readch(ctx);
        }
        if (scan) {
            return json_ok;
        }
        result = (JSON_VALUE *)malloc(sizeof(JSON_VALUE));
        if (!result) {
            snprintf(error_str, 512, "%d:%d: Out of memory", ctx->line, ctx->col);
            return NULL;
        }
        result->tag = JSON_BOOLEAN;
        result->value.boolean = 0;
        return result;
    }
    snprintf(error_str, 512, "%d:%d: Expected value", ctx->line, ctx->col);
    return NULL;
}

static JSON_VALUE *
number(JSON_PARSE_CTX *ctx, int scan)
{
    const char *s = ctx->s + ctx->i;
    int ch;
    JSON_VALUE *result;
    printf("%s %d\n", ctx->s, ctx->i);
    if (peekch(ctx) == '-') {
        readch(ctx);
    }
    if (peekch(ctx) == '0') {
        readch(ctx);
        if (isdigit(peekch(ctx))) {
            snprintf(error_str, 512, "%d:%d: Numbers may not have leading zeros", ctx->line, ctx->col);
            return NULL;
        }
    } else {
        int n = 0;
        while (isdigit(peekch(ctx))) {
            readch(ctx);
            ++n;
        }
        if (n == 0) {
            snprintf(error_str, 512, "%d:%d: Expected number", ctx->line, ctx->col);
            return NULL;
        }
    }
    if (peekch(ctx) == '.') {
        int n = 0;
        readch(ctx);
        while (isdigit(peekch(ctx))) {
            readch(ctx);
            ++n;
        }
        if (n == 0) {
            snprintf(error_str, 512, "%d:%d: Expected number after decimal point", ctx->line, ctx->col);
            return NULL;
        }
    }
    ch = peekch(ctx);
    if (ch == 'e' || ch == 'E') {
        int n = 0;
        readch(ctx);
        ch = peekch(ctx);
        if (ch == '+' || ch == '-') {
            readch(ctx);
        }
        while (isdigit(peekch(ctx))) {
            readch(ctx);
            ++n;
        }
        if (n == 0) {
            snprintf(error_str, 512, "%d:%d: Expected number after exponent", ctx->line, ctx->col);
            return NULL;
        }
    }
    if (scan) {
        return json_ok;
    }
    result = (JSON_VALUE *)malloc(sizeof(JSON_VALUE));
    if (!result) {
        snprintf(error_str, 512, "%d:%d: Out of memory", ctx->line, ctx->col);
        return NULL;
    }
    result->tag = JSON_NUMBER;
    result->value.number = strtod(s, NULL);
    return result;
}

/* Array and Object need this forward decl */
static JSON_VALUE *value(JSON_PARSE_CTX *ctx, int scan);

static JSON_VALUE *
value(JSON_PARSE_CTX *ctx, int scan)
{
    int ch;
    JSON_VALUE *result = NULL;
    space(ctx);
    ch = peekch(ctx);
    switch (ch) {
    case '{':
        break;
    case '[':
        break;
    case '-':
        result = number(ctx, scan);
        break;
    default:
        if (isdigit(ch)) {
            result = number(ctx, scan);
        } else {
            result = word(ctx, scan);
        }
    }
    space(ctx);
    return result;
}

JSON_VALUE *
json_parse(const char *data, size_t size)
{
    JSON_PARSE_CTX ctx;
    ctx.s = data;
    ctx.size = size;
    ctx.i = 0;
    ctx.line = ctx.col = 0;
    error_str[0] = error_str[511] = '\0';
    return value(&ctx, 0);
}

void
json_dump(JSON_VALUE *v)
{
    if (!v) {
        printf("<null ptr>");
        return;
    }
    switch (v->tag) {
    case JSON_NULL:
        printf("(NULL)");
        break;
    case JSON_BOOLEAN:
        printf("(BOOLEAN: %s)", v->value.boolean ? "true" : "false");
        break;
    case JSON_NUMBER:
        printf("(NUMBER: %.2lf)", v->value.number);
        break;
    default:
        printf("(UNKNOWN)");
        break;
    }
}

int main (int argc, char **argv) {
    const char *s = "42.3e10";
    JSON_VALUE *v = json_parse(s, strlen(s));
    json_dump(v); printf("\n");
    json_free(v);
    return 0;
}
    
