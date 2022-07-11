#include "streamablefile.h"

using namespace Mantids::Memory::Streams;

StreamableFile::StreamableFile(int _rd_fd, int _wr_fd)
{
    rd_fd = _rd_fd;
    wr_fd = _wr_fd;
}

bool StreamableFile::streamTo(Memory::Streams::StreamableObject *out, StreamableObject::Status &wrStatUpd)
{
    StreamableObject::Status cur;
    for (;;)
    {
        char buf[4096];
        ssize_t rsize=read(rd_fd,buf,4096);
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
                    out->writeEOF(false);
                    return false;
                }
                else
                {
                    out->writeEOF(true);
                    return true;
                }
            }
            break;
        }
    }
}

StreamableObject::Status StreamableFile::write(const void *buf, const size_t &count, StreamableObject::Status &wrStatUpd)
{
    StreamableObject::Status cur;
    ssize_t x=0;
    if ((x=::write(wr_fd, buf, count)) == -1)
    {
        cur.succeed=wrStatUpd.succeed=setFailedWriteState();
        return cur;
    }
    cur+=(uint64_t)x;
    wrStatUpd+=(uint64_t)x;
    return  cur;
}
