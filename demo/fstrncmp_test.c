/* Test cases:
 *
 * - fewer than n characters left in f
 * - fs < s, same length
 * - fs > s, same length
 * - fs = s
 * - fs < s, different length
 * - fs > s, different length
 * - fs < s, fs is prefix of s
 * - fs > s, s is prefix of fs.
 */

#include "raw_data.c"

int
main(int argc, char **argv)
{
    FILE *f = fopen("test.txt", "rt");
    MNCL_RAW *val;
    printf("ABC < ACD: %d\n", fstrncmp(f, 3, "ACD"));
    printf("DEF > DEE: %d\n", fstrncmp(f, 3, "DEE"));
    printf("GHI = GHI: %d\n", fstrncmp(f, 3, "GHI"));
    printf("JKL < ZZZZZ: %d\n", fstrncmp(f, 3, "ZZZZZ"));
    printf("MNO > AAAAAAA: %d\n", fstrncmp(f, 3, "AAAAAAA"));
    printf("PQR < PQRARGHLE: %d\n", fstrncmp(f, 3, "PQRARGHLE"));
    printf("ST_ > S: %d\n", fstrncmp(f, 3, "S"));
    printf("EOF < YAY: %d\n", fstrncmp(f, 3, "YAY"));
    fclose(f);
    val = zipfile_get_resource("../bin/rawtest.zip", "shadow.txt");
    if (val) {
        printf("%s\n", val->data);
        free(val->data);
        free(val);
    }
    val = zipfile_get_resource("../bin/rawtest.zip", "rawtest.dat");
    val = filesystem_get_resource("resources", "shadow.txt");
    if (val) {
        printf("%s\n", val->data);
    }
    return 0;
}
