#ifndef HIVE7_READER_H
#define HIVE7_READER_H

#include <string>

namespace CX2 { namespace Files { namespace Hive7 {

class FileReader
{
public:
    FileReader();

protected:
    virtual bool internalFormatOpen() = 0;

    bool open();
    bool close();

    bool isOpen();
    bool reOpen();

    bool readUInt64At( uint64_t * val, const uint64_t & position );
    bool writeUInt64At( const uint64_t & val, const uint64_t & position );

    bool readAt( char * buf, const uint32_t & buflen, const uint64_t & position );
    bool writeAt( char * buf, const uint32_t & buflen, const uint64_t & position );
    bool writeAtEnd( char * buf, const uint32_t & buflen );

    FILE * fp;
    std::string filePath;
};

}}}

#endif // HIVE7_READER_H
