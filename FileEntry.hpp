#include <string>
#include <string.h>

struct DirEntry;

struct FileEntry {
    bool isDir;
    const char * data;
    DirEntry * dir;
    size_t sz = 0;
    size_t size();
    FileEntry& operator[](std::string key);
    const FileEntry& operator[](std::string key) const;
    static FileEntry NULL_ENTRY;
};

struct DirEntry {
    const char * name;
    FileEntry file;
};

inline size_t FileEntry::size() {
    if (sz) return sz;
    if (isDir) while (dir[++sz].name);
    else sz = strlen(data);
    return sz;
}
inline FileEntry& FileEntry::operator[](std::string key) {
    for (int i = 0; dir[i].name; i++)
        if (this->dir[i].name == key) return this->dir[i].file;
    return NULL_ENTRY;
}
inline const FileEntry& FileEntry::operator[](std::string key) const {
    for (int i = 0; dir[i].name; i++)
        if (this->dir[i].name == key) return this->dir[i].file;
    return NULL_ENTRY;
}