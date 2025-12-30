#pragma once

#include <stdint.h>
#include <string>
#include <optional>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Mantids30::Memory::Containers {

class FileMap
{
public:
    FileMap();
    ~FileMap();


    FileMap & operator=(FileMap & bc);

    bool mmapDisplace(const size_t &offsetBytes);

    // thanks to larsmans: http://stackoverflow.com/questions/4460507/appending-to-a-memory-mapped-file
    /**
     * @brief mmapAppend Append data to the mmap memory.
     * @param buf data to be appended
     * @param count bytes to be appended.
     * @return -1 if error, or bytes written (can be zero or different to nbytes).
     */
    std::optional<size_t> mmapAppend(void const *buf, const size_t &count);

    /**
     * @brief mmapPrepend Prepend data to the mmap memory.
     * @param buf data to be prepended
     * @param count bytes to be preapended.
     * @return -1 if error, or bytes written (can be zero or different to nbytes).
     */
    std::optional<size_t> mmapPrepend(void const *buf, const size_t &count);

    /**
     * @brief closeFile Close currently openned file.
     * @param respectDeleteFileOnDestruction if false, will not destroy the file at all.
     * @return true if close suceeded
     */
    bool closeFile(bool respectDeleteFileOnDestruction=true);


    // Mmap/FILE MODE methods:
    // TODO: use mmap64... or plain seek mode...
    bool mmapTruncate(const size_t &nSize);

    /**
     * @brief openFile Open file and Map to Memory
     * @param filePath
     * @param readOnly
     * @param createFile
     * @return
     */
    bool openFile(const std::string & filePath, bool readOnly, bool createFile);


    std::string getCurrentFileName() const;

    size_t getFileOpenSize() const;

    char *getMmapAddr() const;

    void setDeleteFileOnDestruction(bool value);

private:

    bool unMapFile();
    bool mapFileUsingCurrentFileDescriptor(size_t len);
    void cleanVars();

    /**
     * @brief currentFileName current filename used.
     */
    std::string currentFileName;
    /**
     * @brief deleteFileOnDestruction Remove the file when this class ends.
     */
    bool deleteFileOnDestruction;
    /**
     * @brief fd mmap file descriptor:
     */
    int fd;
    /**
     * @brief linearMemOriginalPointer original pointer used
     */
    char * mmapAddr;
    /**
     * @brief containerBytesOriginalBytes original container size.
     */
    size_t fileOpenSize;

    /**
     * @brief readOnly Use Read-Only Mode.
     */
    bool readOnly;

#ifdef _WIN32
    HANDLE hFileMapping;
#endif
};

}

