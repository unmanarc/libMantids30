#pragma once

#include "b_base.h"
#include "filemap.h"
#include "b_mem.h"

namespace Mantids30::Memory::Containers {

class B_MMAP: public B_Base
{
public:
    B_MMAP();
    ~B_MMAP() override;
    // TODO: rename

    /**
     * @brief Reference a File in mmap mode.
     * @param filePath file to be referenced.
     * @return true if succeed
     */
    bool referenceFile(const std::string & filePath = "", bool readOnly = false, bool createFile = true);
    /**
     * @brief getCurrentFileName Get current FileName Full Path
     * @return Full path string
     */
    std::string getCurrentFileName() const override;

    /**
     * @brief setDeleteFileOnDestruction set remove file when this object is deleted
     * @param value true for remove the file.
     */
    void setDeleteFileOnDestruction(bool value);

    virtual size_t size() override;
    /**
     * @brief findChar
     * @param c
     * @param offset
     * @return
     */
    std::optional<size_t> findChar(const int & c, const size_t &offset = 0, size_t searchSpace = 0, bool caseSensitive = false) override;

protected:
    /**
     * @brief truncate the current container to n bytes.
     * @param bytes n bytes.
     * @return new container size.
     */
    std::optional<size_t> truncate2(const size_t &bytes) override;
    /**
     * @brief Append more data
     * @param data data to be appended
     * @param len data size in bytes to be appended
     * @param prependMode mode: true will prepend the data, false will append.
     * @return appended bytes
     */
    std::optional<size_t> append2(const void * buf, const size_t &len, bool prependMode = false) override;
    /**
     * @brief remove n bytes at the beggining shrinking the container
     * @param bytes bytes to be removed
     * @return bytes removed.
     */
    std::optional<size_t> displace2(const size_t &bytes = 0) override;
    /**
     * @brief free the whole container
     * @return true if succeed
     */
    bool clear2() override;
    /**
    * @brief Append this current container to a stream.
    * @param out data stream out
    * @param bytes size of data to be copied in bytes. -1 copy all the container but the offset.
    * @param offset displacement in bytes where the data starts.
    * @return
    */
    std::optional<size_t> copyToStream2(std::ostream & out, const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0) override;
    /**
    * @brief Internal Copy function to copy this container to a new one.
    * @param bc Binary container.
    * @param bytes size of data to be copied in bytes. -1 copy all the container but the offset.
    * @param offset displacement in bytes where the data starts.
    * @return
    */
    std::optional<size_t> copyToStreamableObject2(StreamableObject & bc,const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0) override;
    /**
     * @brief Copy append to another binary container.
     * @param bc destination binary container
     * @param bytes size of data in bytes to be copied
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return number of bytes copied (in bytes)
     */
    std::optional<size_t> copyToBuffer2(void * buf, const size_t &count, const size_t &offset = 0) override;
    /**
     * @brief Compare memory with the container
     * @param mem Memory to be compared
     * @param len Memory size in bytes to be compared
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return true where comparison returns equeal.
     */
    bool compare2(const void * buf, const size_t &count, bool caseSensitive = true, const size_t &offset = 0) override;

private:
    void reMapMemoryContainer();

    /**
     * @brief getRandomFileName Create a new random fileName
     * @return random filename with path
     */
    std::string getRandomFileName();
    /**
     * @brief Creates empty file on the filesystem to use it in future.
     * @return true if the file was successfully created at the specified fileName (or produced one)
     */
    bool createEmptyFile(const std::string & fileName);


    // File:
    FileMap fileReference;
    B_MEM mem;
};

}

