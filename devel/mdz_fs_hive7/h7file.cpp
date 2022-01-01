#include "h7file.h"
#include <stdio.h>

#ifndef _WIN32
#include <byteswap.h>
#else
#define bswap_64(x) _byteswap_uint64(x)
#endif

// File Header
#define MAGIC_BANNER_SIZE 6
#define HEADER_SIZE (MAGIC_BANNER_SIZE+8)

using namespace Mantids::Files::Hive7;


H7File::H7File(const std::string &filePath)
{
    this->filePath = filePath;
    currentFileSize = 0;
    fp = 0;
    lastError = H7_NOERROR;
    open();
}

H7File::~H7File()
{
    close();
}

bool H7File::internalFormatOpen()
{
    /*
        Header:
            - 6b: Magic Banner <HIVE7A>
            - 8b: File expected size
    */
    updateCurrentFileSize();

    char magicBanner[MAGIC_BANNER_SIZE] = { 'H','I','V','E','7','A' };
    uint64_t reportedFileSize = HEADER_SIZE;

    if (reportedFileSize == 0)
    {
        writeAt(magicBanner,MAGIC_BANNER_SIZE,0); // Banner
        writeUInt64At(0,MAGIC_BANNER_SIZE); // Dummy Reported File Size
        // TODO: put the empty root here.
        updateCurrentFileSize();
        writeCurrentFileSize();
    }

    if (!readAt(magicBanner,MAGIC_BANNER_SIZE,0))
    {
        lastError = H7_FILECORRUPTED_INVALIDMAGICBYTES;
        return false;
    }
    if (!readUInt64At(&reportedFileSize,MAGIC_BANNER_SIZE))
    {
        lastError = H7_FILECORRUPTED_HEADERERROR;
        return false;
    }
    if ( reportedFileSize != currentFileSize )
    {
        lastError = H7_FILECORRUPTED_INVALIDSIZE;
        return false;
    }

    return true;
}

void H7File::updateCurrentFileSize()
{
    // Check current file size.
    fseeko64(fp, 0L, SEEK_END);
    currentFileSize = ftello64(fp);
    fseeko64(fp, 0L, SEEK_SET);
}

bool H7File::writeCurrentFileSize()
{
    char rawValue[sizeof(uint64_t)];

    int littleEndianChecker = 0x2A;
    if(*(char *)&littleEndianChecker == 0x2A) // Little Endian
        *((uint64_t *)rawValue) = bswap_64(currentFileSize);
    else // Big Endian
        *((uint64_t *)rawValue) = currentFileSize;

    return writeAt( rawValue,sizeof(uint64_t),  MAGIC_BANNER_SIZE);
}

eH7Error H7File::getLastError() const
{
    return lastError;
}
