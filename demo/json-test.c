#include <stdio.h>
#include "../src/json.c"

void json_dump(JSON_VALUE *);

static void
dump_object_node(TREE_NODE *node) {
    KEY_VALUE_NODE *n = (KEY_VALUE_NODE *)node;
    printf("\n\t\"%s\": ", n->key);
    json_dump(n->value);
}


void
json_dump(JSON_VALUE *v)
{
    if (!v) {
        printf("%s\n", json_error());
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
    case JSON_OBJECT:
        printf("(OBJECT:");
        tree_inorder(&v->value.object, dump_object_node);
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
    char *s;
    JSON_VALUE *v;
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
    v = json_parse(s, strlen(s));
    json_dump(v); printf("\n");
    if (argc > 2) {
        int i;
        for (i = 2; i < argc; ++i) {
            JSON_VALUE *val = json_lookup(v, argv[i]);
            printf("%s: ", argv[i]);
            if (!val) {
                printf("Not found");
            } else {
                json_dump(val);
            }
            printf("\n");
        }
    }
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
