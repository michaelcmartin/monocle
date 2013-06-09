#ifndef RESOURCE_H_
#define RESOURCE_H_

#include <vector>
#include <string>
#include <map>

namespace monocle {

    void addResourceDirectory(const std::string &path);
    void addResourceZipFile(const std::string &path);

    typedef std::vector<unsigned char> DataBlob;

    class ResourceData {
    public:
        ResourceData(const std::string &resource);
        virtual ~ResourceData();
        unsigned char* data() const { return &(*data_)[0]; }
        int size() const { return data_->size(); }
        unsigned short decodeShort(int offset) const;
        unsigned int decodeInt(int offset) const;
    private:
        DataBlob* data_;
        std::string resource_;
    };
}

#endif
