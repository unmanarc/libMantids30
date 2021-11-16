#ifndef STREAMABLEPROCESS_H
#define STREAMABLEPROCESS_H

#include <cx2_hlp_functions/appexec.h>
#include "streamable.h"

namespace CX2 { namespace Memory { namespace Streams {

class StreamableProcess : public Memory::Streams::Streamable
{
public:
    StreamableProcess(CX2::Helpers::AppSpawn * spawner);
    ~StreamableProcess();
    /**
     * Retrieve Stream to another Streamable.
     * @param objDst pointer to the destination object.
     * @return false if failed, true otherwise.
     */
    virtual bool streamTo(Memory::Streams::Streamable * out, Status & wrStatUpd) override;
    virtual Status write(const void * buf, const size_t &count, Status & wrStatUpd) override;

private:
    bool streamStdOut,streamStdErr;
    CX2::Helpers::AppSpawn * spawner;
};

}}}

#endif // STREAMABLEPROCESS_H
