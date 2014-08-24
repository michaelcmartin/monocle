#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <zlib.h>
#include "monocle.h"
#include "tree.h"

/* Local utility functions */
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
    int i, result = 0;
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

/* Zipfile seeking functions */
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

/* Core resource-extraction functions */
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
            uint64_t crc = crc32(0L, Z_NULL, 0);
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

    /* Check for path traversal silliness */
    if ((resourcename[0] == '.' && resourcename[1] == '.') || strstr(resourcename, "/..")) {
        return NULL;
    }
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

/* Data structures for handling the resource manager */

typedef enum { PROVIDER_DIRECTORY, PROVIDER_ZIPFILE, NUM_PROVIDER_TYPES } PROVIDER_TYPE;

struct provider {
    struct provider *next;
    PROVIDER_TYPE tag;
    char path[1];
};

struct resmap_node {
    TREE_NODE header;
    const char *resname;
    MNCL_RAW *resource;
    int refcount;
};

static struct provider *providers = NULL;
static TREE locked_resources = { NULL };
static TREE reverse_map = { NULL };

static int
rescmp(TREE_NODE *a, TREE_NODE *b)
{
    return strcmp(((struct resmap_node *)a)->resname, ((struct resmap_node *)b)->resname);
}

static int
ptrcmp(TREE_NODE *a, TREE_NODE *b)
{
    return (intptr_t)((struct resmap_node *)a)->resource - (intptr_t)((struct resmap_node *)b)->resource;
}

/* Not thread safe. */
static struct provider *
make_provider(const char *path, PROVIDER_TYPE ptype)
{
    struct provider *newprov = malloc(sizeof(struct provider)+strlen(path));
    if (!newprov) {
        return NULL;
    }
    newprov->tag = ptype;
    strcpy(newprov->path, path);
    newprov->next = providers;
    providers = newprov;
    return newprov;
}

/* Used elsewhere in the library, but not exported */
void
mncl_uninit_raw_system(void)
{
    while (providers) {
        struct provider *next = providers->next;
        printf ("Unmounting %s: %s\n", providers->tag == PROVIDER_DIRECTORY ? "directory" : "zipfile", providers->path);
        free(providers);
        providers = next;
    }
}

/* The actual, exported functions */
int
mncl_add_resource_directory(const char *path)
{
    return make_provider(path, PROVIDER_DIRECTORY) ? 1 : 0;
}

int
mncl_add_resource_zipfile(const char *path)
{
    return make_provider(path, PROVIDER_ZIPFILE) ? 1 : 0;
}

MNCL_RAW *
mncl_acquire_raw(const char *resource)
{
    MNCL_RAW *result = NULL;
    struct resmap_node seek, *found = NULL;
    struct provider *i = providers;
    char *duped_name;
    seek.resname = resource;
    found = (struct resmap_node *)tree_find(&locked_resources, (TREE_NODE *)&seek, rescmp);
    if (found) {
        ++found->refcount;
        return found->resource;
    }
    while (i && !result) {
        switch (i->tag) {
        case PROVIDER_DIRECTORY:
            result = filesystem_get_resource(i->path, resource);
            break;
        case PROVIDER_ZIPFILE:
            result = zipfile_get_resource(i->path, resource);
            break;
        default:
            /* ? */
            break;
        }
        i = i->next;
    }
    if (!result) {
        /* Don't pollute our resource map */
        return NULL;
    }
    /* Update the resource map */
    found = malloc(sizeof(struct resmap_node));
    duped_name = (char *)malloc(strlen(resource)+1);
    strcpy(duped_name, resource);
    found->resname = duped_name;
    found->resource = result;
    found->refcount = 1;
    tree_insert(&locked_resources, (TREE_NODE *)found, rescmp);
    /* Build another copy for the reverse map */
    found = malloc(sizeof(struct resmap_node));
    duped_name = (char *)malloc(strlen(resource)+1);
    strcpy(duped_name, resource);
    found->resname = duped_name;
    found->resource = result;
    found->refcount = 0;
    tree_insert(&reverse_map, (TREE_NODE *)found, ptrcmp);
    
    return result;
}

void
mncl_release_raw(MNCL_RAW *raw)
{
    struct resmap_node seek, *found = NULL, *found2 = NULL;
    if (!raw) {
        return;
    }
    seek.resource = raw;
    found = (struct resmap_node *)tree_find(&reverse_map, (TREE_NODE *)&seek, ptrcmp);
    if (!found) {
        return;
    }
    printf("Mapped to %s...", found->resname);
    found2 = (struct resmap_node *)tree_find(&locked_resources, (TREE_NODE *)found, rescmp);
    
    if (found2) {
        --found2->refcount;
        if (!found2->refcount) {
            printf("freeing.\n");
            tree_delete(&reverse_map, (TREE_NODE *)found);
            tree_delete(&locked_resources, (TREE_NODE *)found2);
            if (found2->resource->data) {
                free(found2->resource->data);
                free(found2->resource);
                free((void *)found2->resname);
                free((void *)found->resname);
                free(found);
                free(found2);
            }
        } else {
            printf("new refcount %d\n", found2->refcount);
        }
    }
}

/* Exported accessor functions */

int
mncl_raw_size(MNCL_RAW *raw)
{
    return raw->size;
}

uint8_t
mncl_raw_u8(MNCL_RAW *raw, int offset)
{
    return raw->data[offset];
}

uint16_t
mncl_raw_u16le(MNCL_RAW *raw, int offset)
{
    return ((uint16_t)raw->data[offset+1] << 8) | raw->data[offset];
}

uint32_t
mncl_raw_u32le(MNCL_RAW *raw, int offset)
{
    return ((uint32_t)raw->data[offset+3] << 24) |
           ((uint32_t)raw->data[offset+2] << 16) |
           ((uint32_t)raw->data[offset+1] << 8)  |
           raw->data[offset];
}

uint64_t
mncl_raw_u64le(MNCL_RAW *raw, int offset)
{
    return ((uint64_t)raw->data[offset+7] << 56) |
           ((uint64_t)raw->data[offset+6] << 48) |
           ((uint64_t)raw->data[offset+5] << 40) |
           ((uint64_t)raw->data[offset+4] << 32) |
           ((uint64_t)raw->data[offset+3] << 24) |
           ((uint64_t)raw->data[offset+2] << 16) |
           ((uint64_t)raw->data[offset+1] << 8)  |
           raw->data[offset];
}

uint16_t
mncl_raw_u16be(MNCL_RAW *raw, int offset)
{
    return ((uint16_t)raw->data[offset] << 8) | raw->data[offset+1];
}

uint32_t
mncl_raw_u32be(MNCL_RAW *raw, int offset)
{
    return ((uint32_t)raw->data[offset] << 24) |
           ((uint32_t)raw->data[offset+1] << 16) |
           ((uint32_t)raw->data[offset+2] << 8)  |
           raw->data[offset+3];
}

uint64_t
mncl_raw_u64be(MNCL_RAW *raw, int offset)
{
    return ((uint64_t)raw->data[offset] << 56) |
           ((uint64_t)raw->data[offset+1] << 48) |
           ((uint64_t)raw->data[offset+2] << 40) |
           ((uint64_t)raw->data[offset+3] << 32) |
           ((uint64_t)raw->data[offset+4] << 24) |
           ((uint64_t)raw->data[offset+5] << 16) |
           ((uint64_t)raw->data[offset+6] << 8)  |
           raw->data[offset+7];
}

int8_t
mncl_raw_s8(MNCL_RAW *raw, int offset)
{
    return (int8_t)raw->data[offset];
}

int16_t
mncl_raw_s16le(MNCL_RAW *raw, int offset)
{
    return (int16_t)mncl_raw_u16le(raw, offset);
}

int16_t
mncl_raw_s16be(MNCL_RAW *raw, int offset)
{
    return (int16_t)mncl_raw_u16be(raw, offset);
}

int32_t
mncl_raw_s32le(MNCL_RAW *raw, int offset)
{
    return (int32_t)mncl_raw_u32le(raw, offset);
}

int32_t
mncl_raw_s32be(MNCL_RAW *raw, int offset)
{
    return (int32_t)mncl_raw_u32be(raw, offset);
}

int64_t
mncl_raw_s64le(MNCL_RAW *raw, int offset)
{
    return (int64_t)mncl_raw_u64le(raw, offset);
}

int64_t
mncl_raw_s64be(MNCL_RAW *raw, int offset)
{
    return (int64_t)mncl_raw_u64be(raw, offset);
}

float
mncl_raw_f32le(MNCL_RAW *raw, int offset)
{
    union { uint32_t i; float f; } punner;
    punner.i = mncl_raw_u32le(raw, offset);
    return punner.f;
}

float
mncl_raw_f32be(MNCL_RAW *raw, int offset)
{
    union { uint32_t i; float f; } punner;
    punner.i = mncl_raw_u32be(raw, offset);
    return punner.f;
}

double
mncl_raw_f64le(MNCL_RAW *raw, int offset)
{
    union { uint64_t i; double f; } punner;
    punner.i = mncl_raw_u64le(raw, offset);
    return punner.f;
}

double
mncl_raw_f64be(MNCL_RAW *raw, int offset)
{
    union { uint64_t i; double f; } punner;
    punner.i = mncl_raw_u64be(raw, offset);
    return punner.f;
}
