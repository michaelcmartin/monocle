#include <stdio.h>
#include <stdint.h>
#include "monocle.h"

int errors = 0;

#define TEST(fn, offs, val, fmt) \
    printf(#fn ": Expected " fmt ", got " fmt " (%s)\n", val, fn(raw, offs), (fn(raw, offs) == val) ? "OK" : (++errors, "Not OK"))

void
test_string(const char *expected)
{
    MNCL_RAW *raw = mncl_acquire_raw("shadow.txt");
    if (raw) {
        printf ("Expected \"%s\", got \"%s\" (%s)\n", expected, raw->data, strcmp(raw->data, expected) ? (++errors, "Not OK") : "OK");
        mncl_release_raw(raw);
    } else {
        printf ("Could not find shadow.txt\n");
        ++errors;
    }
}

int
main(int argc, char **argv)
{
    MNCL_RAW *raw, *raw2;
    mncl_add_resource_directory("./");
    test_string("RAW");
    mncl_add_resource_zipfile("rawtest.zip");
    test_string("ZIP");
    raw = mncl_acquire_raw("rawtest.dat");
    printf("Size of: char: %d, short: %d, int: %d, long: %d\n", sizeof(char), sizeof(short), sizeof(int), sizeof(long));
    TEST(mncl_raw_u8, 1, 0xf6, "%d");
    TEST(mncl_raw_s8, 1, 0xf6 - 0x100, "%d");
    TEST(mncl_raw_u16be, 1, 0xf6e9, "%d");
    TEST(mncl_raw_s16be, 1, 0xf6e9 - 0x10000, "%d");
    TEST(mncl_raw_u16le, 1, 0xe9f6, "%d");
    TEST(mncl_raw_s16le, 1, 0xe9f6 - 0x10000, "%d");
    TEST(mncl_raw_u32be, 6, (long)0x80bf3ff0, "%ld");
    TEST(mncl_raw_u32le, 6, (long)0xf03fbf80, "%ld");
    TEST(mncl_raw_s32be, 6, (long)0x80bf3ff0 - 0x100000000, "%d");
    TEST(mncl_raw_s32le, 6, 0xf03fbf80L - 0x100000000L, "%d");
    TEST(mncl_raw_u64le, 15, 0x8472916872b02100L, "%lu");
    TEST(mncl_raw_s64le, 15, 0x8472916872b02100L, "%ld");
    TEST(mncl_raw_u64be, 1, 0xf6e979000080bf3fL, "%lu");
    TEST(mncl_raw_s64be, 1, 0xf6e979000080bf3fL, "%lu");
    TEST(mncl_raw_f32be, 0, 0x1.edd2f2p+6, "%7.3f");
    TEST(mncl_raw_f32le, 4, -1.0, "%7.3f");
    TEST(mncl_raw_f64be, 8, 1.0, "%7.3lf");
    TEST(mncl_raw_f64le, 16, 0x1.472916872b021p+9, "%7.3lf");

    raw2 = mncl_acquire_raw("rawtest.dat");
    printf ("Testing resource interning: ");
    if (raw == raw2) {
        printf("OK\n");
    } else {
        ++errors;
        printf("Not OK\n");
    }
    mncl_release_raw(raw2);
    raw2 = mncl_acquire_raw("rawtest.dat");
    printf ("Testing resource ref-counting: ");
    if (raw == raw2) {
        printf("OK\n");
    } else {
        ++errors;
        printf("Not OK\n");
    }
    mncl_release_raw(raw2);
    mncl_release_raw(raw);

    printf("\n%d error%s\n", errors, errors != 1 ? "s" : "");    
    return 0;
}
