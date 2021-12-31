#ifndef HIVE7_H
#define HIVE7_H

#include <string>
#include "filereader.h"
#include "registry.h"
#include "directory.h"

namespace Mantids { namespace Files { namespace Hive7 {

enum eH7Error
{
    H7_NOERROR,
    H7_FILECORRUPTED_INVALIDMAGICBYTES,
    H7_FILECORRUPTED_HEADERERROR,
    H7_FILECORRUPTED_INVALIDSIZE
};

class H7File : private FileReader
{
public:
    H7File( const std::string & filePath );
    ~H7File();

    // TODO: tags
    // TODO:
    Directory openRootDir();

    eH7Error getLastError() const;

protected:
    bool internalFormatOpen();

private:
    void updateCurrentFileSize();
    bool writeCurrentFileSize();

    eH7Error lastError;
    uint64_t currentFileSize;
};

}}}

#endif // HIVE7_H
