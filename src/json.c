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
            if (json->value.array.data[i]) {
                json_free(json->value.array.data[i]);
            }
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

static int
hex_decode(JSON_PARSE_CTX *ctx)
{
    int i, result = 0;
    for (i = 0; i < 4; ++i) {
        int c = readch(ctx);
        int hexit = 0;
        if (c >= '0' && c <= '9') {
            hexit = c - '0';
        } else if (c >= 'A' && c <= 'F') {
            hexit = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            hexit = c - 'a' + 10;
        } else {
            snprintf(error_str, 512, "%d:%d: Expected hex digit", ctx->line, ctx->col);
            return -1;
        }
        result = result * 16 + hexit;
    }
    return result;
}

static int
json_str_size(JSON_PARSE_CTX *ctx_orig, int scan)
{
    /* We don't want to alter the context here, so we make a copy */
    JSON_PARSE_CTX ctx = *ctx_orig;
    int c = readch(&ctx);
    int result = 0;

    if (c != '\"') {
        snprintf(error_str, 512, "%d:%d: Expected string", ctx.line, ctx.col);
        return -1;
    }
    while(1) {
        c = readch(&ctx);
        if (c < 32) {
            snprintf(error_str, 512, "%d:%d: Unterminated string constant", ctx_orig->line, ctx_orig->col);
            return -1;
        }
        if (c == '\"') {
            if (scan) {
                *ctx_orig = ctx;
            }
            return result;
        } else if (c == '\\') {
            c = readch(&ctx);
            if (c < 32) {
                snprintf(error_str, 512, "%d:%d: Unterminated string constant", ctx_orig->line, ctx_orig->col);
                return -1;
            } else {
                int ch;
                switch(c) {
                case 'u':
                    ch = hex_decode(&ctx);
                    if (ch < 0) {
                        return -1;
                    } else if (ch == 0) {
                        snprintf(error_str, 512, "%d:%d: NULL character in string", ctx.line, ctx.col);
                        return -1;
                    } else if (ch < 0x80) {
                        result += 1;
                    } else if (ch < 0x800) {
                        result += 2;
                    } else {
                        result += 3;
                    }
                    break;
                case '\"':
                case '\\':
                case '/':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                    result += 1;
                    break;
                default:
                    snprintf(error_str, 512, "%d:%d: Illegal string escape '%c'", ctx.line, ctx.col, c);
                    return -1;
                }
            }
        } else {
            result += 1;
        }
    }
    return result;
}

/* The buffer dst must have been pre-sized by a call to
 * json_str_size. This function returns 0 on failure, >= 0 on
 * success. */
static int
json_strcpy(char *dst, JSON_PARSE_CTX *ctx) {
    int c = readch(ctx); /* Skip the leading quote */
    while(1) {
        c = readch(ctx);
        if (c == '\"') {
            return 1;
        } else if (c == '\\') {
            int ch;
            c = readch(ctx);
            switch(c) {
            case 'u':
                ch = hex_decode(ctx);
                if (ch < 0) {
                    return 0;
                } else if (ch < 0x80) {
                    *dst++ = ch;
                } else if (ch < 0x800) {
                    *dst++ = (((ch >> 6) & 0xff) | 0xC0);
                    *dst++ = (ch & 0x3f) | 0x80;
                } else {
                    *dst++ = (((ch >> 12) & 0xff) | 0xe0);
                    *dst++ = ((ch >> 6) & 0x3f) | 0x80;
                    *dst++ = (ch & 0x3f) | 0x80;
                }
                break;
            case '\"':
            case '\\':
            case '/':
                *dst++ = c;
                break;
            case 'b':
                *dst++ = '\b';
                break;
            case 'f':
                *dst++ = '\f';
                break;
            case 'n':
                *dst++ = '\n';
                break;
            case 'r':
                *dst++ = '\r';
                break;
            case 't':
                *dst++ = '\t';
                break;
            default:
                snprintf(error_str, 512, "%d:%d: Illegal string escape '%c'", ctx->line, ctx->col, c);
                return 0;
            }
        } else {
            *dst++ = c;
        }
    }
    return 1;
}

static JSON_VALUE *
string(JSON_PARSE_CTX *ctx, int scan)
{
    JSON_STRING_VALUE *result;
    int sz = json_str_size(ctx, scan);
    if (sz < 0) {
        return NULL;
    }
    if (scan) {
        return json_ok;
    }
    result = malloc(sizeof(JSON_STRING_VALUE) + sz + 1);
    if (!result) {
        snprintf(error_str, 512, "%d:%d: Out of memory", ctx->line, ctx->col);
        return NULL;
    }
    result->str[sz] = '\0';
    if (json_strcpy(result->str, ctx)) {
        result->core.tag = JSON_STRING;
        result->core.value.string = result->str;
        return (JSON_VALUE *)result;
    }
    free (result);
    return NULL;
}

/* Array and Object need this forward decl */
static JSON_VALUE *value(JSON_PARSE_CTX *ctx, int scan);

static JSON_VALUE *
array(JSON_PARSE_CTX *ctx, int scan)
{
    JSON_ARRAY_VALUE *result = NULL;
    int i, count = 0;
    JSON_PARSE_CTX start = *ctx;
    int ch = readch(ctx);
    if (ch != '[') {
        snprintf(error_str, 512, "%d:%d: Expected '['", ctx->line, ctx->col);
        return NULL;
    }
    space(ctx);
    while (1) {
        ch = peekch(ctx);
        /* If we aren't just starting, we've just read a 'value',
         * which strips trailing whitespace. The next character should
         * be a ] or a , or the beginning of the first value. */
        if (ch == ']') {
            readch(ctx);
            break;
        } else if (ch == '\0') {
            snprintf(error_str, 512, "%d:%d: Unterminated array", start.line, start.col);
            return NULL;
        }
        if (count) {
            if (ch != ',') {
                snprintf(error_str, 512, "%d:%d: Expected ','", ctx->line, ctx->col);
                return NULL;
            }
            readch(ctx);
        }
        if (!value(ctx, 1)) {
            return NULL;
        }
        ++count;
    }
    if (scan) {
        return json_ok;
    }
    result = malloc(sizeof(JSON_ARRAY_VALUE) + (sizeof (JSON_VALUE *) * count));
    if (!result) {
        snprintf(error_str, 512, "%d:%d: Out of memory", ctx->line, ctx->col);
        return NULL;
    }
    /* This is kind of copy-pastey. We might better served with a
     * split like when parsing strings. */
    *ctx = start;
    readch(ctx); /* Consume the '[' */
    result->core.tag = JSON_ARRAY;
    result->core.value.array.size = count;
    result->core.value.array.data = result->array;
    for (i = 0; i < count; ++i) {
        result->array[i] = NULL;
    }
    space(ctx);
    i = 0;
    while (1) {
        ch = peekch(ctx);
        /* If we aren't just starting, we've just read a 'value',
         * which strips trailing whitespace. The next character should
         * be a ] or a , or the beginning of the first value. */
        if (ch == ']') {
            readch(ctx);
            break;
        } else if (ch == '\0') {
            snprintf(error_str, 512, "%d:%d: Unterminated array", start.line, start.col);
            return NULL;
        }
        if (i) {
            if (ch != ',') {
                snprintf(error_str, 512, "%d:%d: Expected ','", ctx->line, ctx->col);
                return NULL;
            }
            readch(ctx);
        }
        result->array[i] = value(ctx, 0);
        if (!result->array[i]) {
            /* This should really only happen if we run out of memory
             * partway through; any malformed JSON objects should be
             * caught by the sizing scan above. */
            json_free((JSON_VALUE *)result);
            return NULL;
        }
        ++i;
    }
    return (JSON_VALUE *)result;
}

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
        result = array(ctx, scan);
        break;
    case '-':
        result = number(ctx, scan);
        break;
    case '\"':
        result = string(ctx, scan);
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
    case JSON_STRING:
        printf("(STRING: \"%s\")", v->value.string);
        break;
    case JSON_ARRAY:
    {
        int i;
        printf("(ARRAY: Size %d", v->value.array.size);
        for (i = 0; i < v->value.array.size; ++i) {
            printf("\n\t");
            json_dump(v->value.array.data[i]);
        }
        printf(")");
    }
    break;
    default:
        printf("(UNKNOWN)");
        break;
    }
}

static void
test_size(const char *s, int expected) {
    int actual;
    JSON_PARSE_CTX ctx;
    ctx.s = s;
    ctx.size = strlen(s);
    ctx.i = 0;
    ctx.line = ctx.col = 0;
    error_str[0] = error_str[511] = '\0';
    actual = json_str_size(&ctx, 0);
    if (ctx.i || ctx.line || ctx.col) {
        printf("%s: FAILURE: json_str_size advanced our context\n", s);
    }
    if (actual < 0) {
        printf("%s: %s: Error message \"%s\"\n", s, (expected < 0) ? "SUCCESS" : "FAILURE", error_str);
    } else if (actual != expected) {
        printf("%s: FAILURE: Expected %d, got %d\n", s, expected, actual);
    } else {
        printf("%s: SUCCESS\n", s);
    }
}

int main (int argc, char **argv) {
    const char *s = "[\"\\u4e16\\u754c\\u597d\", \"\\u03b1\\u03b2\\u03b3\", \"abc\"]";
    JSON_VALUE *v = json_parse(s, strlen(s));
    json_dump(v); printf("\n");
    json_free(v);
    test_size("abc", -1);
    test_size("\"abc", -1);
    test_size("\"abc\"", 3);
    test_size("\"\"", 0);
    test_size("\"\\abc\"", -1);
    test_size("\"a\\bc\"", 3);
    test_size("\"\\u4f60\\u597d\"", 6);
    return 0;
}
