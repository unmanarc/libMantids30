#include "streamablefile.h"
#include <fcntl.h>

using namespace Mantids::Memory::Streams;

StreamableFile::StreamableFile(int _rd_fd, int _wr_fd)
{
    rd_fd = _rd_fd;
    wr_fd = _wr_fd;
}

StreamableFile::~StreamableFile()
{
    closeAll();
}

int StreamableFile::open(const char *path, int oflag, mode_t __mode)
{
    closeAll();
    int fd = ::open(path,oflag,__mode);

    if ((oflag|O_RDONLY)!=0)
    {
        rd_fd = fd;
        wr_fd = -1;
    }
    if ((oflag|O_WRONLY)!=0)
    {
        rd_fd = -1;
        wr_fd = fd;
    }
    if ((oflag|O_RDWR)!=0)
    {
        rd_fd = fd;
        wr_fd = fd;
    }

    return fd;
}

bool StreamableFile::streamTo(Memory::Streams::StreamableObject *out, StreamableObject::Status &wrStatUpd)
{
    StreamableObject::Status cur;

    // Restart the read from zero (for multiple streamTo)...
    lseek(rd_fd, 0, SEEK_SET);

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

    // Always stick to the EOF
    lseek(rd_fd, 0, SEEK_END);

    if ((x=::write(wr_fd, buf, count)) == -1)
    {
        cur.succeed=wrStatUpd.succeed=setFailedWriteState();
        return cur;
    }
    cur+=(uint64_t)x;
    wrStatUpd+=(uint64_t)x;
    return  cur;
}

void StreamableFile::closeAll()
{
    if (rd_fd!=STDIN_FILENO && rd_fd>0)   close(rd_fd);
    if (wr_fd!=STDOUT_FILENO  && wr_fd!=STDERR_FILENO  && wr_fd>0)  close(wr_fd);

    rd_fd=-1;
    wr_fd=-1;
}
