#include "streamableprocess.h"
#ifndef _WIN32
#include <string.h>

Mantids::Memory::Streams::StreamableProcess::StreamableProcess(Helpers::AppSpawn *spawner)
{
    this->spawner = spawner;
}

Mantids::Memory::Streams::StreamableProcess::~StreamableProcess()
{
    delete this->spawner;
}

bool Mantids::Memory::Streams::StreamableProcess::streamTo(StreamableObject *out, Status &wrStatUpd)
{
    Status cur;
    for (;;)
    {
        auto rsp =spawner->pollResponse();

        // Nothing to read?
        if (rsp.empty()) break;

        char buf[4096];
        memset(buf,0,sizeof(buf));
        ssize_t rsize=-1;

        if (rsp.find(STDOUT_FILENO)!=rsp.end())
            rsize = spawner->read(STDOUT_FILENO,buf,sizeof(buf)-1);
        if (rsp.find(STDERR_FILENO)!=rsp.end())
            rsize = spawner->read(STDERR_FILENO,buf,sizeof(buf)-1);

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

   spawner->waitUntilProcessEnds();
   return true;
}

Mantids::Memory::Streams::StreamableObject::Status Mantids::Memory::Streams::StreamableProcess::write(const void *buf, const size_t &count, Status &wrStatUpd)
{
    // TODO:
    Mantids::Memory::Streams::StreamableObject::Status status;
    status.succeed = false;
    status.finish = false;
    return status;
}
#endif
