#ifndef STREAMABLEFD_H
#define STREAMABLEFD_H

#include "streamableobject.h"
#include <unistd.h>

namespace Mantids { namespace Memory { namespace Streams {

/**
 * @brief The StreamableFile class (NOTE: not thread-safe for R/W)
 */
class StreamableFile : public Memory::Streams::StreamableObject
{
public:
    StreamableFile(int _rd_fd = STDIN_FILENO, int _wr_fd = STDOUT_FILENO);
    ~StreamableFile();

    /**
     * @brief open Open File function
     * @param path Desired Path
     * @param oflag Open Flags (https://linux.die.net/man/3/open)
     * @param __mode File Mode, eg. 0700
     * @return result from system open function
     */
    int open(const char *path, int oflag, mode_t __mode );
    /**
     * @brief streamTo Stream all the content to another StreamableObject
     * @param out output
     * @param wrStatUpd write status counter
     * @return true if streamed ok
     */
    virtual bool streamTo(Memory::Streams::StreamableObject * out, Status & wrStatUpd) override;
    /**
     * @brief write Write/Append into the file
     * @param buf buffer to be written
     * @param count size of the buffed to be written
     * @param wrStatUpd Overall Write Status
     * @return Write Status from the operation
     */
    virtual Status write(const void * buf, const size_t &count, Status & wrStatUpd) override;

private:
    void closeAll();
    int rd_fd,wr_fd;
};

}}}

#endif // STREAMABLEFD_H
