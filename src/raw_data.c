#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <zlib.h>
#include "monocle.h"

static int
decodeInt(unsigned char *p) 
{
    return ((unsigned int)p[3] << 24) | ((unsigned int)p[2] << 16) | ((unsigned int)p[1] << 8) | p[0];
}

static short
loadShort(FILE *f) 
{
    int l = fgetc(f);
    int h = fgetc(f);
    return (unsigned short)((h << 8) | l);
}

static unsigned int
loadInt(FILE *f)
{
    int l = loadShort(f) & 0xFFFF;
    int h = loadShort(f) & 0xFFFF;
    return (h << 16) | l;
}

static void
loadSkip(FILE *f, int n)
{
    int i;
    for (i = 0; i < n; ++i) {
        fgetc(f);
    }
}

/* Like strncmp(n, fs, s) where fs is a string made by reading n
 * characters from f. Always reads n characters from f. s must be
 * null-terminated. fs needn't be and is expected not to be. */
static int
fstrncmp(FILE *f, int n, const char *s)
{
    int i, result;
    for (i = 0; i < n; ++i) {
        int c = fgetc(f);
        if (c < 0) {
            /* We return early here because at EOF we have no more file to burn */
            return -1;
        }
        result = c - (unsigned char)(*s);
        if (result) {
            ++i; ++s;
            break;
        }
        ++s;
    }
    if (!result && i == n && *s) {
        result = -1;
    }
    for (; i < n; ++i) {
        fgetc(f);
    }
    return result;
}

static int
seek_to_central_directory(FILE *f)
{
    long minstart, size, i;
    int state;
    char *buf;
    minstart = 65557;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    if (minstart > size) {
        minstart = size;
    }
    fseek(f, -minstart, SEEK_END);
    buf = (char *)malloc(minstart);
    if (!buf) {
        /* Disastrous memory exhaustion failure. Fragmentation? */
        return 0;
    }
    if ((long)fread(&buf[0], 1, minstart, f) != minstart) {
        /* Couldn't read the file suffix. Probably a premature EOF. */
        /* This really shouldn't happen */
        free(buf);
        return 0;
    }
    state = 0;
    for (i = minstart - 19; i >= 0; --i) {
        switch(buf[i]) {
        case 0x06:
            state = 1;
            break;
        case 0x05:
            state = (state == 1) ? 2 : 0;
            break;
        case 0x4b:
            state = (state == 2) ? 3 : 0;
            break;
        case 0x50:
            if (state == 3) {
                fseek(f, decodeInt((unsigned char *)buf+i+16), SEEK_SET);
                free(buf);
                /* Found it! */
                return 1;
            }
            state = 0;
            break;
        default:
            state = 0;
            break;
        }
    }
    free(buf);
    /* Not a ZIP archive */
    return 0;
}

struct zip_entry {
    int compressedSize, uncompressedSize, compression;
    unsigned int crc32;
};

/* Precondition: f needs to be pointing to the start of the central
 * directory. If you know you haven't reached path yet, you're still
 * OK as long as f is pointing to the start of a central directory
 * record.*/
static int
find_file_in_zip(FILE *f, const char *path, struct zip_entry *ze)
{
    while (1) {
        short fnameLen;
        int magic, suffixLen;
        long offset;
        magic = loadInt(f);
        if (magic == 0x06054b50 || magic == 0x05054b50 || magic == 0x06064b50) {
            /* We've hit the End of Central Directory record, a
             * digital signature block, or a Zip64 end of directory
             * record. We're done, and we didn't find what we were
             * looking for. */
            return 0;
        }
        if (magic != 0x02014b50) {
            /* ZIP directory corrupt */
            return 0;
        }
        loadSkip(f, 6);
        ze->compression = loadShort(f);
        loadSkip(f, 4);
        ze->crc32 = loadInt(f);
        ze->compressedSize = loadInt(f);
        ze->uncompressedSize = loadInt(f);
        fnameLen = loadShort(f);
        suffixLen = loadShort(f);
        suffixLen += loadShort(f);
        loadSkip(f, 8);
        offset = loadInt(f);
        if (!fstrncmp(f, fnameLen, path)) {
            if (ze->compression != 0 && ze->compression != 8) {
                /* Unknown compression type */
                return 0;
            }
            fseek(f, offset, SEEK_SET);
            suffixLen = 0;
            magic = loadInt(f);
            if (magic != 0x04034b50) {
                /* ZIP directory corrupt */
                return 0;
            }
            loadSkip(f, 22);
            suffixLen += loadShort(f);
            suffixLen += loadShort(f);
            loadSkip(f, suffixLen);
            return 1;
        }
        /* That wasn't it. Skip the suffix to get to the next
         * entry */
        loadSkip(f, suffixLen);
    }
    /* This should really qualify as unreachable code */
    return 0;
}

MNCL_RAW *
zipfile_get_resource(const char *pathname, const char *resourcename)
{
    int success = 1;
    FILE *f = NULL;
    void *outbuf = NULL;
    struct zip_entry ze;
    ze.uncompressedSize = 0;
    f = fopen(pathname, "rb");
    if (!f) {
        success = 0;
    }
    if (success) {
        success = seek_to_central_directory(f);
    }
    if (success) {
        success = find_file_in_zip(f, resourcename, &ze);
    }
    if (success) {
        /* We actually found the file, and we know enough about it to
         * perform the extraction! */
        outbuf = malloc(ze.uncompressedSize);
        printf ("Extracting %s: %d -> %d bytes\n", resourcename, ze.compressedSize, ze.uncompressedSize);
        if (ze.compression) {
            int leftToRead = ze.compressedSize;
            int ret, index;
            z_stream strm;
            unsigned char inbuf[8192];
            /* allocate inflate state */
            index = 0;
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            strm.avail_in = 0;
            strm.next_in = Z_NULL;
            ret = inflateInit2(&strm, -MAX_WBITS);
            if (ret != Z_OK) {
                success = 0;
            }
            while (success && leftToRead > 0) {
                int nRead = fread(inbuf, 1, (leftToRead > 8192) ? 8192 : leftToRead, f);
                leftToRead -= nRead;
                strm.avail_in = nRead;
                strm.next_in = inbuf;
                strm.avail_out = ze.uncompressedSize - index;
                strm.next_out = (unsigned char *)((char *)outbuf + index);
                ret = inflate(&strm, Z_NO_FLUSH);
                switch (ret) {
                case Z_NEED_DICT:
                case Z_STREAM_ERROR:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    success = 0;
                    break;
                case Z_STREAM_END:
                    leftToRead = 0;
                    break;
                default:
                    break;
                }
                /* We might be able to ignore index and just not
                 * update strm.avail_out and strm.next_out here. */
                index = ze.uncompressedSize - strm.avail_out;
            }
            inflateEnd(&strm);
        } else {
            if ((long)fread(outbuf, 1, ze.compressedSize, f) != ze.compressedSize) {
                /* Can't read the resource for some reason */
                success = 0;
            }
        }
        if (success) {
            unsigned long crc = crc32(0L, Z_NULL, 0);
            crc = crc32(crc, (unsigned char*)outbuf, ze.uncompressedSize);
            crc &= 0xFFFFFFFF;
            if (ze.crc32 != crc) {
                success = 0;
            }
        }
    }

    /* Function epilogue. Clean up all our resources. */
    if (f) {
        fclose(f);
    }
    if (!success && outbuf) {
        free(outbuf);
        outbuf = NULL;
    }
    if (outbuf) {
        MNCL_RAW *result = malloc(sizeof(MNCL_RAW));
        if (result) {
            result->data = outbuf;
            result->size = ze.uncompressedSize;
            return result;
        } else {
            free(outbuf);
        }
    }
    return NULL;
}

MNCL_RAW *
filesystem_get_resource(const char *pathbase, const char *resourcename)
{
    char buf[PATH_MAX];
    FILE *f;
    int i, j;
    size_t size;
    MNCL_RAW *result;

    /* Distressingly, it's kind of easier and safer to roll our own
     * combination of strncpy and strncat than to try to coerce the C
     * library to do what we need */
    for (i = 0; i < PATH_MAX; ++i) {
        if (!pathbase[i]) {
            break;
        }
        buf[i] = pathbase[i] == '\\' ? '/' : pathbase[i];
    }
    if (i == 0 || (i != PATH_MAX && buf[i-1] != '/')) {
        buf[i] = '/';
        ++i;
    }
    for (j = 0; i < PATH_MAX; ++i, ++j) {
        buf[i] = resourcename[j] == '\\' ? '/' : resourcename[j];
        if (!resourcename[j]) {
            break;
        }
    }
    buf[PATH_MAX-1] = 0;
    /* Check for path traversal silliness */
    if ((buf[0] == '.' && buf[1] == '.') || strstr(buf, "/..")) {
        return NULL;
    }
    f = fopen(buf, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    result = malloc(sizeof(MNCL_RAW));
    if (result) {
        result->data = malloc(size);
        result->size = size;
        if (result->data) {
            for (i = 0; i < size; ++i) {
                result->data[i] = fgetc(f);
            }
        } else {
            free(result);
            result = NULL;
        }
    }
    fclose (f);
    return result;
}
