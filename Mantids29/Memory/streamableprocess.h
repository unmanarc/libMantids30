#pragma once

#ifndef _WIN32

#include <Mantids29/Helpers/appexec.h>
#include "streamableobject.h"

namespace Mantids29 { namespace Memory { namespace Streams {

class StreamableProcess : public Memory::Streams::StreamableObject
{
public:
    StreamableProcess(Mantids29::Helpers::AppSpawn * spawner);
    ~StreamableProcess();
    /**
     * Retrieve Stream to another Streamable.
     * @param objDst pointer to the destination object.
     * @return false if failed, true otherwise.
     */
    virtual bool streamTo(Memory::Streams::StreamableObject * out, Status & wrStatUpd) override;
    virtual Status write(const void * buf, const size_t &count, Status & wrStatUpd) override;

private:
    bool streamStdOut,streamStdErr;
    Mantids29::Helpers::AppSpawn * spawner;
};

}}}
#endif

