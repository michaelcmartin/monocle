#ifndef JSON_H_
#define JSON_H_

#include <stdlib.h>
#include "tree.h"

typedef enum {
    JSON_NULL,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JSON_TYPE;

typedef struct json_value_ {
    JSON_TYPE tag;
    union {
        int boolean;
        double number;
        char *string;
        struct { int size; struct json_value_ **data; } array;
        TREE object;
    } value;
} JSON_VALUE;

JSON_VALUE *json_parse(const char *data, size_t size);
const char *json_error();
JSON_VALUE *json_lookup(JSON_VALUE *map, const char *key);
void json_free (JSON_VALUE *json);

#endif
