#pragma once

#ifndef _WIN32

#include <Mantids30/Helpers/appexec.h>
#include "streamableobject.h"

namespace Mantids30 { namespace Memory { namespace Streams {

class StreamableProcess : public Memory::Streams::StreamableObject
{
public:
    StreamableProcess(Mantids30::Helpers::AppSpawn * spawner);
    ~StreamableProcess();
    /**
     * Retrieve Stream to another Streamable.
     * @param objDst pointer to the destination object.
     * @return false if failed, true otherwise.
     */
    virtual bool streamTo(Memory::Streams::StreamableObject * out, WriteStatus & wrStatUpd) override;
    virtual WriteStatus write(const void * buf, const size_t &count, WriteStatus & wrStatUpd) override;

private:
    bool m_streamStdOut,m_streamStdErr;
    Mantids30::Helpers::AppSpawn * m_spawner;
};

}}}
#endif

