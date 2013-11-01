#include <stdio.h>
/* We #define MONOCULAR to nothing here because we're using bits of
 * Monocle as a statically linked component. */
#define MONOCULAR
#include "../src/json.c"

void mncl_data_dump(MNCL_DATA *);

static void
dump_object_node(const char *key, void *value, void *unused) {
    (void)unused;
    printf("\n\t\"%s\": ", key);
    mncl_data_dump((MNCL_DATA *)value);
}

void
mncl_data_dump(MNCL_DATA *v)
{
    if (!v) {
        printf("%s\n", mncl_data_error());
        return;
    }
    switch (v->tag) {
    case MNCL_DATA_NULL:
        printf("(NULL)");
        break;
    case MNCL_DATA_BOOLEAN:
        printf("(BOOLEAN: %s)", v->value.boolean ? "true" : "false");
        break;
    case MNCL_DATA_NUMBER:
        printf("(NUMBER: %.2lf)", v->value.number);
        break;
    case MNCL_DATA_STRING:
        printf("(STRING: \"%s\")", v->value.string);
        break;
    case MNCL_DATA_ARRAY:
    {
        int i;
        printf("(ARRAY: Size %d", v->value.array.size);
        for (i = 0; i < v->value.array.size; ++i) {
            printf("\n\t");
            mncl_data_dump(v->value.array.data[i]);
        }
        printf(")");
    }
    break;
    case MNCL_DATA_OBJECT:
        printf("(OBJECT:");
        mncl_kv_foreach(v->value.object, dump_object_node, NULL);
        printf(")");
        break;
    default:
        printf("(UNKNOWN)");
        break;
    }
}

static void
test_size(const char *s, int expected) {
    int actual;
    MNCL_DATA_PARSE_CTX ctx;
    ctx.s = s;
    ctx.size = strlen(s);
    ctx.i = 0;
    ctx.line = ctx.col = 0;
    error_str[0] = error_str[511] = '\0';
    actual = mncl_data_str_size(&ctx, 0);
    if (ctx.i || ctx.line || ctx.col) {
        printf("%s: FAILURE: mncl_data_str_size advanced our context\n", s);
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
    char *s;
    MNCL_DATA *v, *d;
    s = calloc(65535, 1);
    if (argc > 1) {
        FILE *f = fopen(argv[1], "r");
        if (f) {
            size_t size = fread(s, 1, 65534, f);
            fclose(f);
            s[size] = 0;
        } else {
            printf("Could not open %s\n", argv[1]);
            return 1;
        }
    } else {
        printf("No file specified\n");
        return 1;
    }
    v = mncl_parse_data(s, strlen(s));
    d = mncl_data_clone(v);
    mncl_free_data(v);
    mncl_data_dump(d); printf("\n");
    if (argc > 2) {
        int i;
        for (i = 2; i < argc; ++i) {
            MNCL_DATA *val = mncl_data_lookup(d, argv[i]);
            printf("%s: ", argv[i]);
            if (!val) {
                printf("Not found");
            } else {
                mncl_data_dump(val);
            }
            printf("\n");
        }
    }
    mncl_free_data(d);
    test_size("abc", -1);
    test_size("\"abc", -1);
    test_size("\"abc\"", 3);
    test_size("\"\"", 0);
    test_size("\"\\abc\"", -1);
    test_size("\"a\\bc\"", 3);
    test_size("\"\\u4f60\\u597d\"", 6);
    free(s);
    return 0;
}
