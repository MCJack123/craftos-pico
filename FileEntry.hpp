#include <string>
#include <string.h>

struct DirEntry;

struct FileEntry {
    bool isDir;
    const char * data;
    const DirEntry * dir;
    size_t sz = 0;
    size_t size() const;
    const FileEntry& operator[](std::string key) const;
    static const FileEntry NULL_ENTRY;
};

struct DirEntry {
    const char * name;
    const FileEntry file;
};

inline size_t FileEntry::size() const {
    return sz;
}
inline const FileEntry& FileEntry::operator[](std::string key) const {
    for (int i = 0; dir[i].name; i++)
        if (this->dir[i].name == key) return this->dir[i].file;
    return NULL_ENTRY;
}