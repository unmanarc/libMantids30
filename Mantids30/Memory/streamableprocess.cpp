#include "streamableprocess.h"
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

bool Mantids30::Memory::Streams::StreamableProcess::streamTo(StreamableObject * out, WriteStatus &wrStatUpd)
{
    WriteStatus cur;
    for (;;)
    {
        auto rsp =m_spawner->pollResponse();

        // Nothing to read?
        if (rsp.empty()) break;

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
            out->writeEOF(false);
            return false;
        case 0:
            out->writeEOF(true);
            return true;
        default:
            if (!(cur=out->writeFullStream(buf,rsize,wrStatUpd)).succeed || cur.finish)
            {
                if (!cur.succeed)
                {
                    // Transmission error
                    out->writeEOF(false);
                    return false;
                }
                else
                {
                    // Maybe the receiving object is finished...
                    out->writeEOF(true);
                    return true;
                }
            }
            break;
        }
    }

   m_spawner->waitUntilProcessEnds();

   return true;
}

Mantids30::Memory::Streams::WriteStatus Mantids30::Memory::Streams::StreamableProcess::write(const void *buf, const size_t &count, WriteStatus &wrStatUpd)
{
    // TODO:
    Mantids30::Memory::Streams::WriteStatus status;
    status.succeed = false;
    status.finish = false;
    return status;
}
#endif
