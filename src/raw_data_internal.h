#ifndef RAW_INTERNAL_H_
#define RAW_INTERNAL_H_

// This must be included only in C++ sources.

#include <string>
#include <vector>
#include "monocle.h"

namespace monocle {
    namespace resource {
        // Core provider class. The Resource Manager internal to
        // resource.cpp will use this as its interface type.
        class ResourceProvider {
        public:
            virtual ~ResourceProvider() {}
            // Returns true if the resource was provided, returns false otherwise
            virtual bool get(const std::string &resource, std::vector<unsigned char>& out) = 0;
        };

        // A ResourceProvider that reads stuff out of files. Implemented in resource.cpp.
        class DirectoryProvider : public ResourceProvider {
        public:
            DirectoryProvider(const std::string& path);
            virtual ~DirectoryProvider() {}
            virtual bool get(const std::string &resource, std::vector<unsigned char>& out);
        private:
            const std::string path_;
        };

        // A ResourceProvider that reads stuff out of zip files
        struct ZipEntry {
            long offset;
            int compressedSize, uncompressedSize;
            unsigned int crc32;
            bool compressed;
        };

        typedef std::map<std::string, ZipEntry> ZipIndex;

        class ZipFileProvider : public ResourceProvider {
        public:
            ZipFileProvider(const std::string &fname);
            virtual ~ZipFileProvider();
            virtual std::vector<std::string> contents();
            virtual bool get(const std::string& resourceName, std::vector<unsigned char>& out);
        private:
            void init_zip_index();
            FILE *fp_;
            ZipIndex index_;
        };
    }
}
#endif
