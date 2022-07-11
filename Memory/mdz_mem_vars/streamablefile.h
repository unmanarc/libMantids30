#ifndef STREAMABLEFD_H
#define STREAMABLEFD_H

#include "streamableobject.h"
#include <unistd.h>

namespace Mantids { namespace Memory { namespace Streams {

class StreamableFile : public Memory::Streams::StreamableObject
{
public:
    StreamableFile(int _rd_fd = STDIN_FILENO, int _wr_fd = STDOUT_FILENO);
    /**
     * Retrieve Stream to another Streamable.
     * @param objDst pointer to the destination object.
     * @return false if failed, true otherwise.
     */
    virtual bool streamTo(Memory::Streams::StreamableObject * out, Status & wrStatUpd) override;
    virtual Status write(const void * buf, const size_t &count, Status & wrStatUpd) override;

private:
    int rd_fd,wr_fd;
};

}}}

#endif // STREAMABLEFD_H
