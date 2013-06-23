#include <stdio.h>
#include <zlib.h>
#include <map>
#include "raw_data_internal.h"
#include "exception.h"

namespace {
    unsigned short decodeShort(unsigned char *p) {
        return ((unsigned short)p[1] << 8) | p[0];
    }

    unsigned int decodeInt(unsigned char *p) {
        return ((unsigned int)p[3] << 24) | ((unsigned int)p[2] << 16) | ((unsigned int)p[1] << 8) | p[0];
    }

    unsigned short loadShort(FILE *f) {
        int l = fgetc(f);
        int h = fgetc(f);
        if (l < 0 || h < 0) {
            throw monocle::panic ("Unexpected EOF");
        }
        return (unsigned short)((h << 8) | l);
    }

    unsigned int loadInt(FILE *f) {
        unsigned int l = loadShort(f);
        unsigned int h = loadShort(f);
        return (h << 16) | l;
    }

    void loadSkip(FILE *f, int n) {
        for (int i = 0; i < n; ++i) {
            fgetc(f);
        }
    }

    unsigned int central_directory_offset(FILE *f)
    {
        long minstart = 65557;
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        if (minstart > size) {
            minstart = size;
        }
        fseek(f, -minstart, SEEK_END);
        std::vector<unsigned char> buf(minstart);
        if ((long)fread(&buf[0], 1, minstart, f) != minstart) {
            throw monocle::panic("Error reading file tail");
        }
        int state = 0;
        for (long i = minstart - 19; i >= 0; --i) {
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
                    return decodeInt(&buf[i+16]);
                }
                state = 0;
                break;
            default:
                state = 0;
                break;
            }
        }
        throw monocle::panic("Not a ZIP archive");		    
    }
}

namespace monocle {
    namespace resource {
        void
        ZipFileProvider::init_zip_index() {
            fseek(fp_, central_directory_offset(fp_), SEEK_SET);
            while (true) {
                ZipEntry e;
                short h, fnameLen;
                int magic, suffixLen;
                magic = loadInt(fp_);
                if (magic == 0x06054b50 || magic == 0x05054b50 || magic == 0x06064b50) {
                    // We've hit the End of Central Directory record, a
                    // digital signature block, or a Zip64 end of directory
                    // record. We're done, though we might be in trouble later
                    // if this is really an encrypted or Zip64 archive.
                    break;
                }
                if (magic != 0x02014b50) {
                    throw monocle::panic("ZIP directory corrupt");
                }
                loadSkip(fp_, 6);
                h = loadShort(fp_);
                if (h == 8) {
                    e.compressed = true;
                } else if (h == 0) {
                    e.compressed = false;
                } else {
                    throw monocle::panic("Unknown compression type");
                }
                loadSkip(fp_, 4);
                e.crc32 = loadInt(fp_);
                e.compressedSize = loadInt(fp_);
                e.uncompressedSize = loadInt(fp_);
                fnameLen = loadShort(fp_);
                suffixLen = loadShort(fp_);
                suffixLen += loadShort(fp_);
                loadSkip(fp_, 8);
                e.offset = loadInt(fp_);
                std::vector<char> buf(fnameLen);
                if ((long)fread (&buf[0], 1, fnameLen, fp_) != fnameLen) {
                    throw monocle::panic("Can't read directory information");
                }
                index_[std::string(&buf[0], fnameLen)] = e;
                loadSkip(fp_, suffixLen);
            }
            // Before we proceed, we should now confirm all the file offsets and
            // update them to point at the actual data.
            for (ZipIndex::iterator i = index_.begin(); i != index_.end(); ++i) {
                int magic, suffLen;
                suffLen = 30;
                fseek(fp_, i->second.offset, SEEK_SET);
                magic = loadInt(fp_);
                if (magic != 0x04034b50) {
                    throw monocle::panic("ZIP directory corrupt");
                }
                loadSkip(fp_, 22);
                suffLen += loadShort(fp_);
                suffLen += loadShort(fp_);
                i->second.offset += suffLen;
            }
        }


        ZipFileProvider::ZipFileProvider(const std::string& fname)
        {
            fp_ = fopen(fname.c_str(), "rb");
            if (!fp_) {
                throw monocle::panic("Could not open "+fname);
            }
            init_zip_index();
        }

        ZipFileProvider::~ZipFileProvider()
        {
            fclose(fp_);
        }

        std::vector<std::string>
        ZipFileProvider::contents()
        {
            std::vector<std::string> result;
            for (ZipIndex::iterator it = index_.begin(); it != index_.end(); ++it) {
                result.push_back(it->first);
            }
            return result;
        }

        bool
        ZipFileProvider::get(const std::string& resourceName, std::vector<unsigned char>& out) {
            ZipIndex::const_iterator it = index_.find(resourceName);
            if (it == index_.end()) {
                return false;
            }
            printf ("Extracting %s: %d -> %d bytes\n", resourceName.c_str(), it->second.compressedSize, it->second.uncompressedSize);
            out.resize(it->second.uncompressedSize);
            fseek(fp_, it->second.offset, SEEK_SET);
            if (it->second.compressed) {
                std::vector<unsigned char> inbuf(8192);
                int leftToRead = it->second.compressedSize;
                int ret, index;
                z_stream strm;
                /* allocate inflate state */
                index = 0;
                strm.zalloc = Z_NULL;
                strm.zfree = Z_NULL;
                strm.opaque = Z_NULL;
                strm.avail_in = 0;
                strm.next_in = Z_NULL;
                ret = inflateInit2(&strm, -MAX_WBITS);
                if (ret != Z_OK) {
                    throw monocle::panic("Could not allocate inflate state");
                }
                while (leftToRead > 0) {
                    int nRead = fread(&inbuf[0], 1, (leftToRead > 8192) ? 8192 : leftToRead, fp_);
                    leftToRead -= nRead;
                    strm.avail_in = nRead;
                    strm.next_in = &inbuf[0];
                    strm.avail_out = it->second.uncompressedSize - index;
                    strm.next_out = (unsigned char *)(&out[index]);
                    ret = inflate(&strm, Z_NO_FLUSH);
                    switch (ret) {
                    case Z_NEED_DICT:
                    case Z_STREAM_ERROR:
                    case Z_DATA_ERROR:
                    case Z_MEM_ERROR:
                        (void)inflateEnd(&strm);
                        throw monocle::panic("Corrupt zipfile data");
                    case Z_STREAM_END:
                        (void)inflateEnd(&strm);
                        {
                            unsigned long crc = crc32(0L, Z_NULL, 0);
                            crc = crc32(crc, (unsigned char*)&out[0], out.size());
                            crc &= 0xFFFFFFFF;
                            if (it->second.crc32 != crc) {
                                throw monocle::panic("CRC check failed: file corrupt");
                            }
                        }
                        return true;
                    default:
                        break;
                    }
                    // We might be able to ignore index and just not update strm.avail_out and strm.next_out here.
                    index = it->second.uncompressedSize - strm.avail_out;
                }
                inflateEnd(&strm);
                unsigned long crc = crc32(0L, Z_NULL, 0);
                crc = crc32(crc, (unsigned char*)&out[0], out.size());
                crc &= 0xFFFFFFFF;
                if (it->second.crc32 != crc) {
                    throw monocle::panic("CRC check failed: file corrupt");
                }
            } else {
                if ((long)fread(&out[0], 1, it->second.compressedSize, fp_) != it->second.compressedSize) {
                    throw monocle::panic("Can't read resource \"" + resourceName + "\"");
                }
                unsigned long crc = crc32(0L, Z_NULL, 0);
                crc = crc32(crc, (unsigned char*)&out[0], out.size());
                crc &= 0xFFFFFFFF;
                if (it->second.crc32 != crc) {
                    throw monocle::panic("CRC check failed: file corrupt");
                }
            }
            return true;
        }
    }
}
