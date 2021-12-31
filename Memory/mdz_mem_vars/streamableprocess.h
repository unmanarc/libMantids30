#ifndef STREAMABLEPROCESS_H
#define STREAMABLEPROCESS_H

#ifndef _WIN32

#include <mdz_hlp_functions/appexec.h>
#include "streamable.h"

namespace Mantids { namespace Memory { namespace Streams {

class StreamableProcess : public Memory::Streams::Streamable
{
public:
    StreamableProcess(Mantids::Helpers::AppSpawn * spawner);
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
    Mantids::Helpers::AppSpawn * spawner;
};

}}}
#endif

#endif // STREAMABLEPROCESS_H
