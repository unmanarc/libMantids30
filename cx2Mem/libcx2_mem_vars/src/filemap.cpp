#include "filemap.h"
#include <cx2_hlp_functions/mem.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#ifndef _WIN32
#include <sys/mman.h>
#else
#include <io.h>
#define MAP_FAILED	((void *) -1)

#ifdef __USE_FILE_OFFSET64
# define LOADDR(v) (0xFFFFFFFF&(v))
# define HIADDR(v) (v>>0x20)
#else
# define LOADDR(v) (v)
# define HIADDR(v) (0x0)
#endif

#endif

using namespace CX2::Memory::Containers;

/**
 * @brief emptyMap Virtual Memory Space used for empty file maps..
 */
static char emptyMap[1] = {0};


FileMap::FileMap()
{
    cleanVars();
}

FileMap::~FileMap()
{
    closeFile();
}

FileMap &FileMap::operator=(FileMap &bc)
{
    cleanVars();

    currentFileName = bc.currentFileName;
    removeOnDestroy = bc.removeOnDestroy;
    fd = bc.fd;
    mmapAddr = bc.mmapAddr;
    fileOpenSize = bc.fileOpenSize;
    readOnly = bc.readOnly;
#ifdef WIN32
    hFileMapping = bc.hFileMapping;
#endif
    bc.cleanVars();

    return *this;
}

void FileMap::cleanVars()
{
    readOnly = false;
    currentFileName = "";
    removeOnDestroy = false;
    fd = -1;
    mmapAddr = nullptr;
#ifdef WIN32
    hFileMapping = nullptr;
#endif
    fileOpenSize=0;
}

void FileMap::setRemoveOnDestroy(bool value)
{
    removeOnDestroy = value;
}

bool FileMap::unMapFile()
{
    bool ret = true;
    // Check if mmap is present, is not invalid and is not the emptyMap, so we can unmap it.
    if (mmapAddr && mmapAddr!=MAP_FAILED && mmapAddr!=emptyMap)
    {
#ifndef _WIN32
        ret = munmap(mmapAddr,fileOpenSize)==0;
#else
        ret = UnmapViewOfFile(mmapAddr)!=0;
#endif
    }

#ifdef WIN32
    if (hFileMapping)
    {
        CloseHandle(hFileMapping);
        hFileMapping = nullptr;
    }
#endif

    mmapAddr = nullptr;
    return ret;
}

bool FileMap::mapFileUsingCurrentFileDescriptor(size_t len)
{
    if (fd == -1)
        return false;

    // Establish the new file size.
    this->fileOpenSize = len;

    // In case we are mapping 0-bytes file:
    if (len == 0)
    {
        // No map for zero bytes!
        this->fileOpenSize = 0;
        this->mmapAddr = emptyMap;
        return true;
    }

#ifndef _WIN32
    this->mmapAddr = static_cast<char *>(mmap(nullptr, len, (readOnly? PROT_READ : PROT_READ | PROT_WRITE), MAP_SHARED, fd, 0));
    if (this->mmapAddr == MAP_FAILED)
    {
        closeFile();
        return false;
    }
#else
    HANDLE hFileDescriptor = (HANDLE)_get_osfhandle(fd);
    if ((hFileMapping=CreateFileMapping(hFileDescriptor,
                             NULL,
                             (readOnly? PAGE_READONLY : PAGE_READWRITE),
                             HIADDR(len), LOADDR(len),
                             NULL)) == NULL)
    {
        // Can't map the file...
        closeFile();
        return false;
    }
    if ((this->mmapAddr=(char *)MapViewOfFile(hFileMapping,
                                              readOnly? FILE_MAP_READ : FILE_MAP_ALL_ACCESS,
                                              0,0,
                                              len)) == NULL)
    {
        // Can't map the file...
        CloseHandle(hFileMapping);
        closeFile();
        return false;
    }
#endif

    return true;
}

char *FileMap::getMmapAddr() const
{
    return mmapAddr;
}

uint64_t FileMap::getFileOpenSize() const
{
    return fileOpenSize;
}

std::string FileMap::getCurrentFileName() const
{
    return currentFileName;
}

bool FileMap::mmapDisplace(const uint64_t &offsetBytes)
{
    CX2::Helpers::Mem::memmove64(mmapAddr, mmapAddr+offsetBytes, fileOpenSize-offsetBytes);
    return mmapTruncate(fileOpenSize-offsetBytes);
}

std::pair<bool, uint64_t> FileMap::mmapAppend(const void *buf, const uint64_t &count)
{
    if (!count) return std::make_pair(true,0);
    /////////////////////////////

    uint64_t curOpenSize = fileOpenSize;
    if (!mmapTruncate(fileOpenSize+count)) return std::make_pair(false,(uint64_t)0);

    memcpy(mmapAddr+curOpenSize,buf,count);
    return std::make_pair(true,count);
}

std::pair<bool, uint64_t> FileMap::mmapPrepend(const void *buf, const uint64_t &count)
{
    if (!count) return std::make_pair(true,0);

    /////////////////////////////
    uint64_t curOpenSize = fileOpenSize;
    if (!mmapTruncate(fileOpenSize+count)) return std::make_pair(false,(uint64_t)0);
    CX2::Helpers::Mem::memmove64(mmapAddr,mmapAddr+count,curOpenSize);
    CX2::Helpers::Mem::memcpy64(mmapAddr,buf,count);
    return std::make_pair(true,count);
}

bool FileMap::closeFile(bool respectRemoveOnDestroy)
{
    // If there is a mmap, close the mmap:
    unMapFile();

    // If there is a file descriptor, close the file descriptor:
    if (fd!=-1) close(fd);

    // If there is an order to destroy the file, destroy the file.
    if (removeOnDestroy && respectRemoveOnDestroy && currentFileName.size())
    {
        remove(currentFileName.c_str());
    }

    // Clean the vars...
    cleanVars();

    return true;
}

bool FileMap::mmapTruncate(const uint64_t &nSize)
{
    // Check if file descriptor is invalid (not file opened)
    if (fd==-1 || readOnly)
        return false;

    // Unmap the file:
    if (!unMapFile())
    {
        // Error unmaping, closing...
        closeFile();
        return false;
    }

    // Truncate the file...
    if (ftruncate64(fd,nSize)!=0)
    {
        closeFile();
        return false;
    }

    // Map the file into memory:
    return mapFileUsingCurrentFileDescriptor(nSize);
}

bool FileMap::openFile(const std::string &filePath, bool readOnly, bool createFile)
{
    // Two consecutive openFile will close the first one (and clean the variables):
    closeFile();

    // Open the file descriptor:
    struct stat64 sbuf;
    int oflags = readOnly? O_RDONLY : (createFile? O_RDWR | O_APPEND | O_CREAT : O_RDWR | O_APPEND );
    if ((fd = open(filePath.c_str(), oflags, 0600)) == -1)
    {
        return false;
    }

    // Use the new parameters:
    this->currentFileName = filePath;
    this->readOnly = readOnly;

    // Retrieve the file stats, specially the size:
    if (stat64(filePath.c_str(), &sbuf) == -1)
    {
        closeFile();
        return false;
    }

    // Map the file into memory:
    return mapFileUsingCurrentFileDescriptor(sbuf.st_size);
}
