#include "streamableprocess.h"
#include <optional>

#ifndef _WIN32
#include <string.h>

Mantids30::Memory::Streams::StreamableProcess::StreamableProcess(Helpers::AppSpawn *spawner)
{
    this->m_spawner = spawner;
}

Mantids30::Memory::Streams::StreamableProcess::~StreamableProcess()
{
    delete this->m_spawner;
}

bool Mantids30::Memory::Streams::StreamableProcess::streamTo(StreamableObject * out)
{
    for (;;)
    {
        auto rsp =m_spawner->pollResponse();

        // Nothing to read?
        if (rsp.empty())
        {
            break;
        }

        char buf[4096];
        memset(buf,0,sizeof(buf));
        ssize_t rsize=-1;

        if (rsp.find(STDOUT_FILENO)!=rsp.end())
            rsize = m_spawner->read(STDOUT_FILENO,buf,sizeof(buf)-1);

        if (rsp.find(STDERR_FILENO)!=rsp.end())
            rsize = m_spawner->read(STDERR_FILENO,buf,sizeof(buf)-1);

        // TODO: if multiple streams are going in, and you close one, the other one should keep transmitting...
        switch (rsize)
        {
        case -1:
            // The process failed... output is not reliable...
            out->writeEOF();
            return false;
        case 0:
            return out->writeEOF();
            break;
        default:
            if (!out->writeFullStream(buf,rsize))
            {
                return false;
            }
            break;
        }

        if (!out->writeStatus.succeed)
        {
            // Transmission error, stop doing anything... (TODO: close the program?)
            return false;
        }
    }

    m_spawner->waitUntilProcessEnds();
    return true;
}

std::optional<size_t> Mantids30::Memory::Streams::StreamableProcess::write(const void *buf, const size_t &count)
{
    // TODO: how to write into the process??
    writeStatus+=-1;
    return std::nullopt;
}
#endif
