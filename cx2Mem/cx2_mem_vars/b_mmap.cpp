#include "b_mmap.h"
#include <time.h>
#include <sys/timeb.h>
#include <random>

using namespace CX2::Memory::Containers;

#ifdef _WIN32
#define FS_DIRSLASH "\\"
#else
#define FS_DIRSLASH "/"
#endif

B_MMAP::B_MMAP()
{
    storeMethod = BC_METHOD_FILEMMAP;
    readOnly = false;
    setContainerBytes(0);
    B_MMAP::clear2();
}

B_MMAP::~B_MMAP()
{
    B_MMAP::clear2();
}

bool B_MMAP::referenceFile(const std::string &filePath, bool readOnly, bool createFile)
{
    B_MMAP::clear2();
    FileMap bcr;

    // Open the file:
    if (!bcr.openFile( filePath.size()? filePath : getRandomFileName(), readOnly, createFile))
    {
        // Failed to open the referenced file.

        /*        if () O_CREAT should do the job for us.
        // Create one.
        if (!createEmptyFile(bcr.currentFileName)) return false;

        // Open it again.
        if (!bcr.openFile(bcr.currentFileName)) return false;*/
        return false;
    }

    // Destroy current memory and assign reference values:
    mem.reference(bcr.getMmapAddr(),bcr.getFileOpenSize());

    // Copy file descriptor and other things.
    fileReference = bcr;

    return true;
}

void B_MMAP::setRemoveOnDestroy(bool value)
{
    fileReference.setRemoveOnDestroy(value);
}

uint64_t B_MMAP::size() const
{
    // TODO: check
//    std::cout << "B_MMAP::  Getting size() " << mem.size() << std::endl << std::flush;
    return mem.size();
}

std::pair<bool, uint64_t> B_MMAP::findChar(const int &c, const uint64_t &offset, uint64_t searchSpace, bool caseSensitive)
{
    if (caseSensitive && !(c>='A' && c<='Z') && !(c>='a' && c<='z') )
        caseSensitive = false;
    return mem.findChar(c,offset,searchSpace, caseSensitive);
}

std::pair<bool, uint64_t> B_MMAP::truncate2(const uint64_t &bytes)
{
    if (!fileReference.mmapTruncate(bytes))
    {
        clear();
        return std::make_pair(false,(uint64_t)0);
    }

    reMapMemoryContainer();
    return std::make_pair(false,size());
}

std::string B_MMAP::getCurrentFileName() const
{
    return fileReference.getCurrentFileName();
}

std::pair<bool, uint64_t> B_MMAP::append2(const void *buf, const uint64_t &len, bool prependMode)
{
    std::pair<bool, uint64_t> addedBytes;

    if (prependMode)
        addedBytes = fileReference.mmapPrepend(buf,len);
    else
        addedBytes = fileReference.mmapAppend(buf,len);

    if (!addedBytes.first)
    {
        //clear(); // :(
        return addedBytes;
    }

    reMapMemoryContainer();

    return addedBytes;
}

std::pair<bool, uint64_t> B_MMAP::displace2(const uint64_t &roBytesToDisplace)
{
    uint64_t bytesToDisplace = roBytesToDisplace;

    if (bytesToDisplace>fileReference.getFileOpenSize())
        return std::make_pair(false,(uint64_t)0);
    if (!fileReference.mmapDisplace(bytesToDisplace))
        return std::make_pair(false,(uint64_t)0);

    reMapMemoryContainer();
    return std::make_pair(true,bytesToDisplace);
}

bool B_MMAP::clear2()
{
    return fileReference.closeFile();
}

std::pair<bool, uint64_t> B_MMAP::copyToStream2(std::ostream &out, const uint64_t &bytes, const uint64_t &offset)
{
    return mem.copyToStream(out,bytes,offset);
}

std::pair<bool, uint64_t> B_MMAP::copyTo2(Streamable &bc, Streams::Status & wrStatUpd, const uint64_t &bytes, const uint64_t &offset)
{
    return mem.appendTo(bc,wrStatUpd,bytes,offset);
}

std::pair<bool,uint64_t> B_MMAP::copyOut2(void *buf, const uint64_t &count, const uint64_t &offset)
{
    return mem.copyOut(buf,count,offset);
}

bool B_MMAP::compare2(const void *buf, const uint64_t &count, bool caseSensitive, const uint64_t &offset)
{
    return this->mem.compare(buf,count,caseSensitive,offset);
}

void B_MMAP::reMapMemoryContainer()
{
    setContainerBytes(fileReference.getFileOpenSize());
    mem.reference(fileReference.getMmapAddr(),size());
}

std::string B_MMAP::getRandomFileName()
{
    std::string::size_type length = 16;
    char baseChars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string randomStr;
    std::mt19937 rg{std::random_device{}()};
    std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(baseChars)-2);
    randomStr.reserve(length);
    while(length--) randomStr += baseChars[pick(rg)];

    return fsDirectoryPath + FS_DIRSLASH + fsBaseFileName + "." + randomStr;
}

bool B_MMAP::createEmptyFile(const std::string &)
{
    // Creates file here.
//    FileMap bcr;
    // TODO:
    /*
    if (!access(fileName.c_str(), F_OK)) return false;

    // Open the file:
    if (!bcr.openFile(fileName))
    {
        // Failed to open the referenced file.
        return false;
    }

    // Don't remove/close on destroy... ;)...
    bcr.fd = -1;

    return true;*/
    return false;
}

