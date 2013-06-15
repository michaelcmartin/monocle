#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>
#include <list>
#include <string>
#include "exception.h"
#include "raw_data_internal.h"

namespace monocle {
    namespace resource {
        typedef std::map<std::string, MNCL_RAW *> ResMap;
        typedef std::map<MNCL_RAW *, std::string> ReverseResMap;
        typedef std::list<ResourceProvider *> Providers;

        class ResourceManager {
        public:
            void addDirectoryProvider(const std::string &path);
            void addZipProvider(const std::string &path);
            ~ResourceManager();
            MNCL_RAW *acquire(const std::string &resource);
            void release(MNCL_RAW *raw);

            static ResourceManager *instance();
        private:
            Providers providers_;
            ResMap locked_resources_;
            ReverseResMap reverse_map_;
            std::map<std::string, int> refcounts_;

            static ResourceManager *instance_;
        };

        DirectoryProvider::DirectoryProvider(const std::string& path)
            : path_(path)
        {
        }

        bool
        DirectoryProvider::get(const std::string &resource, std::vector<unsigned char>& out)
        {
            FILE *f = fopen((path_ + resource).c_str(), "rb");
            if (!f) {
                return false;
            }
            out.clear();
            while (true) {
                int c = fgetc(f);
                if (c == -1) {
                    break;
                }
                out.push_back((unsigned char) c);
            }
            fclose (f);
            return true;
        }

        void
        ResourceManager::addDirectoryProvider(const std::string &path)
        {
            providers_.push_front(new DirectoryProvider(path));
        }

        void ResourceManager::addZipProvider(const std::string &path)
        {
            providers_.push_front(new ZipFileProvider(path));
        }

        ResourceManager* ResourceManager::instance_ = NULL;

        ResourceManager*
        ResourceManager::instance() {
            if (!instance_) {
                instance_ = new ResourceManager();
            }
            return instance_;
        }

        ResourceManager::~ResourceManager()
        {
            for (Providers::iterator i = providers_.begin(); i != providers_.end(); ++i) {
                delete *i;
            }
            for (ResMap::iterator i = locked_resources_.begin(); i != locked_resources_.end(); ++i) {
                free(i->second->data);
                free(i->second);
            }
        }

        MNCL_RAW *
        ResourceManager::acquire(const std::string &resource)
        {
            ResMap::iterator it = locked_resources_.find(resource);
            if (it == locked_resources_.end()) {
                bool success = false;
                for (Providers::iterator i = providers_.begin(); i != providers_.end(); ++i) {
                    try {
                        std::vector<unsigned char> contents;
                        if ((*i)->get(resource, contents)) {
                            MNCL_RAW *result = (MNCL_RAW *)malloc(sizeof(MNCL_RAW));
                            if (result) {                            
                                result->size = contents.size();
                                result->data = (unsigned char *)malloc(result->size);
                                if (result->data) {
                                    success = true;
                                    memcpy(result->data, &(contents[0]), result->size);
                                    locked_resources_[resource] = result;
                                    reverse_map_[result] = resource;
                                    refcounts_[resource] = 0;
                                    break;
                                } else {
                                    free(result);
                                }
                            }
                        }
                    } catch (std::exception&) {
                        // It wasn't in this provider, and something also went horribly wrong.
                        // TODO: Should probably log something special in this case
                    }
                }
                if (!success) {
                    return NULL;
                }
            }
            refcounts_[resource] += 1;
            return locked_resources_[resource];
        }

        void
        ResourceManager::release(MNCL_RAW *raw)
        {
            ReverseResMap::iterator it = reverse_map_.find(raw);
            if (it != reverse_map_.end()) {
                std::string resource = it->second;
                if (--refcounts_[resource] == 0) {
                    free(raw->data);
                    free(raw);
                    reverse_map_.erase(it);
                    locked_resources_.erase(resource);
                    refcounts_.erase(resource);
                }
            }
        }
    }
}

int
mncl_add_resource_directory(const char *path)
{
    try {
        monocle::resource::ResourceManager::instance()->addDirectoryProvider(path);
    } catch (std::exception &e) {
        fprintf(stderr, "WARNING: Could not add resource directory %s: %s\n", path, e.what());
        return 1; // TODO: ERROR CODES
    }
    return 0; // success
}

int
mncl_add_resource_zipfile(const char *path)
{
    try {
        monocle::resource::ResourceManager::instance()->addZipProvider(path);
    } catch (std::exception &e) {
        fprintf(stderr, "WARNING: Could not add ZIP file %s: %s\n", path, e.what());
        return 1;
    }
    return 0;
}

MNCL_RAW *
mncl_acquire_raw(const char *resource)
{
    return monocle::resource::ResourceManager::instance()->acquire(resource);
}

void
mncl_release_raw(MNCL_RAW *raw)
{
    monocle::resource::ResourceManager::instance()->release(raw);
}

int
mncl_raw_size(MNCL_RAW *raw)
{
    return raw->size;
}

unsigned char
mncl_raw_u8(MNCL_RAW *raw, int offset)
{
    return raw->data[offset];
}

unsigned short
mncl_raw_u16le(MNCL_RAW *raw, int offset)
{
    return ((unsigned short)raw->data[offset+1] << 8) | raw->data[offset];
}

unsigned int 
mncl_raw_u32le(MNCL_RAW *raw, int offset)
{
    return ((unsigned int)raw->data[offset+3] << 24) | 
           ((unsigned int)raw->data[offset+2] << 16) | 
           ((unsigned int)raw->data[offset+1] << 8)  |
           raw->data[offset];
}

unsigned long
mncl_raw_u64le(MNCL_RAW *raw, int offset)
{
    return ((unsigned long)raw->data[offset+7] << 56) | 
           ((unsigned long)raw->data[offset+6] << 48) | 
           ((unsigned long)raw->data[offset+5] << 40) | 
           ((unsigned long)raw->data[offset+4] << 32) | 
           ((unsigned long)raw->data[offset+3] << 24) | 
           ((unsigned long)raw->data[offset+2] << 16) | 
           ((unsigned long)raw->data[offset+1] << 8)  |
           raw->data[offset];
}

unsigned short
mncl_raw_u16be(MNCL_RAW *raw, int offset)
{
    return ((unsigned short)raw->data[offset] << 8) | raw->data[offset+1];
}

unsigned int 
mncl_raw_u32be(MNCL_RAW *raw, int offset)
{
    return ((unsigned int)raw->data[offset] << 24) | 
           ((unsigned int)raw->data[offset+1] << 16) | 
           ((unsigned int)raw->data[offset+2] << 8)  |
           raw->data[offset+3];
}

unsigned long
mncl_raw_u64be(MNCL_RAW *raw, int offset)
{
    return ((unsigned long)raw->data[offset] << 56) | 
           ((unsigned long)raw->data[offset+1] << 48) | 
           ((unsigned long)raw->data[offset+2] << 40) | 
           ((unsigned long)raw->data[offset+3] << 32) | 
           ((unsigned long)raw->data[offset+4] << 24) | 
           ((unsigned long)raw->data[offset+5] << 16) | 
           ((unsigned long)raw->data[offset+6] << 8)  |
           raw->data[offset+7];
}

char
mncl_raw_s8(MNCL_RAW *raw, int offset)
{
    return (char)raw->data[offset];
}

short
mncl_raw_s16le(MNCL_RAW *raw, int offset)
{
    return (short)mncl_raw_u16le(raw, offset);
}

short
mncl_raw_s16be(MNCL_RAW *raw, int offset)
{
    return (short)mncl_raw_u16be(raw, offset);
}

int
mncl_raw_s32le(MNCL_RAW *raw, int offset)
{
    return (int)mncl_raw_u32le(raw, offset);
}

int
mncl_raw_s32be(MNCL_RAW *raw, int offset)
{
    return (int)mncl_raw_u32be(raw, offset);
}

long
mncl_raw_s64le(MNCL_RAW *raw, int offset)
{
    return (long)mncl_raw_u64le(raw, offset);
}

long
mncl_raw_s64be(MNCL_RAW *raw, int offset)
{
    return (long)mncl_raw_u64be(raw, offset);
}

float
mncl_raw_f32le(MNCL_RAW *raw, int offset)
{
    union { unsigned int i; float f; } punner;
    punner.i = mncl_raw_u32le(raw, offset);
    return punner.f;
}

float
mncl_raw_f32be(MNCL_RAW *raw, int offset)
{
    union { unsigned int i; float f; } punner;
    punner.i = mncl_raw_u32be(raw, offset);
    return punner.f;
}

double
mncl_raw_f64le(MNCL_RAW *raw, int offset)
{
    union { unsigned long i; double f; } punner;
    punner.i = mncl_raw_u64le(raw, offset);
    return punner.f;
}

double
mncl_raw_f64be(MNCL_RAW *raw, int offset)
{
    union { unsigned long i; double f; } punner;
    punner.i = mncl_raw_u64be(raw, offset);
    return punner.f;
}

