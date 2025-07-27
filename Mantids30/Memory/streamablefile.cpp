#include "streamablefile.h"
#include "Mantids30/Helpers/safeint.h"
#include <fcntl.h>
#include <optional>

using namespace Mantids30::Memory::Streams;

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

bool StreamableFile::streamTo(Memory::Streams::StreamableObject * out)
{
    // Restart the read from zero (for multiple streamTo)...
    lseek(rd_fd, 0, SEEK_SET);

    for (;;)
    {
        char buf[8192];
        ssize_t rsize=read(rd_fd,buf,8192);
        switch (rsize)
        {
        case -1:
            // Unexpected error during file read... don't process with EOF. (bad file)
            out->writeEOF();
            return false;
        case 0:
            // Write EOF into the stream. Maybe some buffer bytes are written down
            return out->writeEOF();
        default:
            if ( !out->writeFullStream(buf,rsize) )
            {
                return false;
            }
            break;
        }
    }
}

std::optional<size_t> StreamableFile::write(const void *buf, const size_t &count)
{
    ssize_t x=0;

    // Always stick to the EOF
    lseek(rd_fd, 0, SEEK_END);

    x=::write(wr_fd, buf, count);
    writeStatus+=x;

    if (x>=0)
        return x;
    else
        return std::nullopt; // Error reflected also in the WriteStatus
}

void StreamableFile::closeAll()
{
    if (rd_fd!=STDIN_FILENO && rd_fd>0)
        close(rd_fd);
    if (wr_fd!=STDOUT_FILENO  && wr_fd!=STDERR_FILENO  && wr_fd>0)
        close(wr_fd);

    rd_fd=-1;
    wr_fd=-1;
}
