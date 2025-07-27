#include "b_mmap.h"
#include <optional>
#include <sys/timeb.h>
#include <random>

using namespace Mantids30::Memory::Containers;

#ifdef _WIN32
#define FS_DIRSLASH "\\"
#else
#define FS_DIRSLASH "/"
#endif

B_MMAP::B_MMAP()
{
    m_storeMethod = BC_METHOD_FILEMMAP;
    m_readOnly = false;
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
        if (!createEmptyFile(bcr.currentFileName)) 
        return false;

        // Open it again.
        if (!bcr.openFile(bcr.currentFileName)) 
        return false;*/
        return false;
    }

    // Destroy current memory and assign reference values:
    mem.reference(bcr.getMmapAddr(),bcr.getFileOpenSize());

    // Copy file descriptor and other things.
    fileReference = bcr;

    return true;
}

void B_MMAP::setDeleteFileOnDestruction(bool value)
{
    fileReference.setDeleteFileOnDestruction(value);
}

size_t B_MMAP::size()
{
    // TODO: check
//    std::cout << "B_MMAP::  Getting size() " << mem.size() << std::endl << std::flush;
    return mem.size();
}

std::optional<size_t> B_MMAP::findChar(const int &c, const size_t &offset, size_t searchSpace, bool caseSensitive)
{
    if (caseSensitive && !std::isalpha(c)) 
    {
        caseSensitive = false;
    }
    return mem.findChar(c,offset,searchSpace, caseSensitive);
}

std::optional<size_t> B_MMAP::truncate2(const size_t &bytes)
{
    if (!fileReference.mmapTruncate(bytes))
    {
        clear();
        return std::nullopt;
    }

    reMapMemoryContainer();
    throw std::runtime_error("Please, don't truncate MMAP Files!");
    return std::nullopt;
}

std::string B_MMAP::getCurrentFileName() const
{
    return fileReference.getCurrentFileName();
}

std::optional<size_t> B_MMAP::append2(const void *buf, const size_t &len, bool prependMode)
{
    std::optional<size_t> addedBytes;

    if (prependMode)
        addedBytes = fileReference.mmapPrepend(buf,len);
    else
        addedBytes = fileReference.mmapAppend(buf,len);

    if (addedBytes==std::nullopt)
    {
        //clear(); // :(
        // Failed.
        return addedBytes;
    }

    reMapMemoryContainer();

    return addedBytes;
}

std::optional<size_t> B_MMAP::displace2(const size_t &roBytesToDisplace)
{
    size_t bytesToDisplace = roBytesToDisplace;


    if (bytesToDisplace>fileReference.getFileOpenSize())
        return std::nullopt;

    if (!fileReference.mmapDisplace(bytesToDisplace))
        return std::nullopt;

    reMapMemoryContainer();

    return bytesToDisplace;
}

bool B_MMAP::clear2()
{
    return fileReference.closeFile();
}

std::optional<size_t> B_MMAP::copyToStream2(std::ostream &out, const size_t &bytes, const size_t &offset)
{
    return mem.copyToStream(out,bytes,offset);
}

std::optional<size_t> B_MMAP::copyToStreamableObject2(StreamableObject &bc, const size_t &bytes, const size_t &offset)
{
    return mem.appendTo(bc,bytes,offset);
}

std::optional<size_t> B_MMAP::copyToBuffer2(void *buf, const size_t &count, const size_t &offset)
{
    return mem.copyOut(buf,count,offset);
}

bool B_MMAP::compare2(const void *buf, const size_t &count, bool caseSensitive, const size_t &offset)
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

    while(length--)
        randomStr += baseChars[pick(rg)];

    return m_fsDirectoryPath + FS_DIRSLASH + m_fsBaseFileName + "." + randomStr;
}

bool B_MMAP::createEmptyFile(const std::string &)
{
    // Creates file here.
//    FileMap bcr;
    // TODO:
    /*
    if (!access(fileName.c_str(), F_OK)) 
        return false;

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

