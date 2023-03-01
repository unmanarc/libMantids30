#include "filereader.h"

#ifndef _WIN32
#include <byteswap.h>
#else
#define bswap_64(x) _byteswap_uint64(x)
#endif

#include <limits>

using namespace Mantids::Files::Hive7;

FileReader::FileReader()
{
}

bool FileReader::open()
{
    if (fp != nullptr) return false;
    fp = fopen64(filePath.c_str(),"w+");

    if (fp == nullptr) return false;

    return internalFormatOpen();
}

bool FileReader::readUInt64At(uint64_t *val, const uint64_t &position)
{
    char rawValue[sizeof(uint64_t)];
    if (!readAt(rawValue,sizeof(uint64_t),position)) return false;

    int littleEndianChecker = 0x2A;
    if(*(char *)&littleEndianChecker == 0x2A) // Little Endian
        *val = bswap_64(*((uint64_t *)rawValue));
    else // Big Endian
        *val = *((uint64_t *)rawValue);

    return true;
}

bool FileReader::writeUInt64At(const uint64_t &val, const uint64_t &position)
{
    char rawValue[sizeof(uint64_t)];

    int littleEndianChecker = 0x2A;
    if(*(char *)&littleEndianChecker == 0x2A) // Little Endian
        *((uint64_t *)rawValue) = bswap_64(val);
    else // Big Endian
        *((uint64_t *)rawValue) = val;

    return writeAt(rawValue,sizeof (uint64_t),position);
}

bool FileReader::readAt(char *buf, const uint32_t &buflen, const uint64_t &position)
{
    if (fseeko64(fp,position,SEEK_SET)!=0) return false;
    return (fread(buf,buflen,1,fp)==buflen);
}

bool FileReader::writeAt(char *buf, const uint32_t &buflen, const uint64_t &position)
{
    if ( fseeko64(fp,position,SEEK_SET)!=0) return false;
    return (fwrite(buf,buflen,1,fp)==buflen);
}

bool FileReader::writeAtEnd(char *buf, const uint32_t &buflen)
{
    if (fseeko64(fp,0,SEEK_END)!=0) return false;
    return (fwrite(buf,buflen,1,fp)==buflen);
}

bool FileReader::isOpen()
{
    return (fp != nullptr);
}

bool FileReader::reOpen()
{
    if (fp != nullptr) close();
    return open();
}

bool FileReader::close()
{
    if (fp == nullptr) return false;
    fclose(fp);
    return true;
}

