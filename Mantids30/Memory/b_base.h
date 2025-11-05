#pragma once


#include "b_chunk.h"

#include "streamableobject.h"

#include <limits.h>
#include <memory>
#include <stdio.h>
#include <list>
#include <vector>
#include <iostream>
#include <optional>
#include <Mantids30/Helpers/mem.h>

namespace Mantids30 { namespace Memory { namespace Containers {

enum BinaryContainerMethod {
    BC_METHOD_CHUNKS,
    BC_METHOD_MEM,
    BC_METHOD_BCREF,
    BC_METHOD_FILEMMAP,
    BC_METHOD_NULL
};

class B_Base : public Streams::StreamableObject
{
public:
    B_Base();
    virtual ~B_Base() override;
    /**
     * @brief operator = copy the data of a container into another container (does not copy limits or flags like read only)
     * @param bc element to be copied.
     */
    B_Base & operator=(B_Base & bc);

    /**
     * @brief print print to stdout...
     */
    void print(FILE * f = stdout);

    // Data Agregattion method method:
    /**
     * @brief Prepend linear memory to this container.
     * @param buf Pointer to the memory buffer to prepend.
     * @return A pair where the first element is a boolean indicating success or failure, and the second element is the number of bytes prepended.
     */
    std::optional<size_t> prepend(const void * buf);
    /**
     * @brief Append null-terminated linear memory to this container.
     * @param buf Pointer to the null-terminated memory buffer to append.
     * @return A pair where the first element is a boolean indicating success or failure, and the second element is the number of bytes appended.
     */
    std::optional<size_t> append(const void * buf);
    /**
    * @brief Append linear memory to this container.
    * @param buf Pointer to the memory buffer to append.
    * @param len Number of bytes in the memory buffer to append.
    * @return A pair where the first element is a boolean indicating success or failure, and the second element is the number of bytes appended.
    */
    std::optional<size_t> append(const void * buf, size_t len);
    /**
     * @brief Prepend linear memory to this container.
     * @param buf Pointer to the memory buffer to prepend.
     * @param len Number of bytes in the memory buffer to prepend.
     * @return A pair where the first element is a boolean indicating success or failure, and the second element is the number of bytes prepended.
     */
    std::optional<size_t> prepend(const void * buf, size_t len);
    // Memory shrinking..
    /**
     * @brief remove n bytes at the beggining shrinking the container
     * @param bytes bytes to be removed
     * @return bytes removed.
     */
    std::optional<size_t> displace(const size_t &bytes = 0);
    /**
     * @brief truncate the current container to n bytes.
     * @param bytes n bytes.
     * @return new container size.
     */
    std::optional<size_t> truncate(const size_t &bytes);
    /**
     * @brief free the whole container
     * @return true if succeed
     */
    bool clear();
    // Copy mechanisms:
    /**
     * @brief Copies data from this container to another until a specified byte sequence is found or a maximum size is reached.
     * 
     * This function will copy data from the current container into the destination container up to the point where the specified
     * delimiter (needle) is found. If the needle is not found within the maximum copy size, it will copy up to maxCopySize bytes.
     * The function also provides an option to remove the delimiter from the source container if it is found.
     *
     * @param destination Reference to the binary container where the copied data will be stored.
     * @param needle Pointer to the memory buffer containing the byte sequence (delimiter) to search for.
     * @param needleLenght Length of the delimiter in bytes.
     * @param maxCopySize Maximum number of bytes to copy from the source container.
     * @param removeNeedle Boolean flag indicating whether to remove the delimiter from the source container if it is found.
     * @return Integer return code:
     *         - 0: Delimiter was found and data copied successfully.
     *         - -1: Failed to copy data (general error).
     *         - -2: Maximum copy size reached without finding the delimiter.
     */
    int copyUntil(B_Base & destination, const void * needle, const size_t &needleLenght, const size_t &maxCopySize, bool removeNeedle = false );
    /**
     * @brief displaceUntil move until some byte sequence
     * @param destination Binary container where the resulted data will be saved.
     * @param needle delimiter
     * @param needleCount delimiter length in bytes
     * @param maxCopySize maximum bytes to be retrieved in bytes.
     * @return retr return codes [0:found, -1:failed, not found, -2:failed, out of size]
     */
    int displaceUntil(B_Base & destination, const void * needle, const size_t &needleCount, const size_t &maxCopySize, bool removeNeedle = false );
    /**
     * @brief displaceUntil move until any of the specified byte sequences
     * @param destination Binary container where the resulting data will be saved.
     * @param needles List of delimiters to search for.
     * @param maxCopySize Maximum bytes to be retrieved in bytes.
     * @param removeNeedle If true, removes the found needle from the original container.
     * @return Return codes [0:found, -1:failed, not found, -2:failed, out of size]
     */
    int displaceUntil(B_Base & destination, const std::list<std::string> needles, const size_t &maxCopySize, bool removeNeedle = true );
    /**
     * @brief freeSplitList Free a list of binary containers.
     * @param x List of binary containers to be freed.
     */
    static void freeSplitList(std::list<Memory::Containers::B_Base *> x);
    /**
    * @brief Append this current container to a new one.
    * @param out Binary container.
    * @param bytes size of data to be copied in bytes. -1 copy all the container but the offset.
    * @param offset displacement in bytes where the data starts.
    * @return
    */
    std::optional<size_t> copyToStream(std::ostream &out, size_t bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0);
    /**
    * @brief Append this current container to a new one. (without reporting EOF)
    * @param bc Binary container.
    * @param bytes size of data to be copied in bytes. -1 copy all the container but the offset.
    * @param offset displacement in bytes where the data starts.
    * @return -1 if error, 0 if no data appended (eg. max reached), n bytes appended.
    */
    std::optional<size_t> appendTo(StreamableObject & out, const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0);
    /**
     * @brief Copy append to another binary container.
     * @param bc destination binary container
     * @param bytes size of data in bytes to be copied
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return number of bytes copied (in bytes) or std::nullopt if the operation was not successful.
     */
    std::optional<size_t> copyOut(void * buf, size_t bytes, const size_t &offset = 0);
    /**
     * @brief Copy the container to an std::string
     * @param bytes bytes to copy (std::numeric_limits<size_t>::max(): all bytes)
     * @param offset offset displacement
     * @return bytes copied or std::numeric_limits<size_t>::max() if error.
     */
    std::optional<size_t> copyToString(std::string & str,size_t bytes = std::numeric_limits<size_t>::max(), const size_t & offset = 0);

    /**
     * @brief toString create string with the data contained here.
     * @param bytes bytes to copy (std::numeric_limits<size_t>::max(): all bytes)
     * @param offset offset displacement
     * @return string containing the data.
     */
    std::optional<std::string> toString(size_t bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0);


    std::string toStringEx(size_t bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0)
    {
        auto r = toString(bytes,offset);
        return r?*r:"";
    }

    /**
     * @brief toUInt64 Convert container data to 64-bit unsigned integer
     * @param base eg. 10 for base10 numeric, and 16 for hex
     * @param bytes bytes to copy (std::numeric_limits<size_t>::max(): all bytes)
     * @param offset offset displacement
     * @return std::numeric_limits<size_t>::max() if error, n if converted.
     */
    std::optional<uint64_t> toUInt64(int base=10, const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0);
    /**
     * @brief toUInt32 Convert container data to 32-bit unsigned integer
     * @param base eg. 10 for base10 numeric, and 16 for hex
     * @param bytes bytes to copy (std::numeric_limits<size_t>::max(): all bytes)
     * @param offset offset displacement
     * @return ULONG_MAX if error, n if converted.
     */
    std::optional<uint32_t> toUInt32(int base = 10, const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0);

    // Needles / Comparison:
    /**
     * @brief Compare memory with the container
     * @param mem Memory to be compared
     * @param len Memory size in bytes to be compared
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return true where comparison returns equeal.
     */
    bool compare(const void * mem, const size_t &len, bool caseSensitive = true, const size_t &offset = 0 );
    /**
     * @brief Compare memory with the container
     * @param cmpString string to be compared
     * @param caseSensitive do a case sensitive comparison
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return true where comparison returns equeal.
     */
    bool compare(const std::string & cmpString, bool caseSensitive = false, const size_t &offset = 0 );

    /**
     * @brief findChar Find the position of a character within the container.
     *
     * This function searches for the first occurrence of a specified character within the container, starting from an optional
     * offset. If a search space is provided, the search will be limited to that many bytes starting from the offset. The search
     * can be case-sensitive or case-insensitive depending on the parameter.
     *
     * @param c Character to find.
     * @param offset Starting position (in bytes) for the search. Default is 0 (start of the container).
     * @param searchSpace Maximum number of bytes to search from the starting offset. If set to 0, the entire remaining container will be searched.
     * @param caseSensitive If true, the search is case-sensitive; otherwise, it is case-insensitive.
     * @return A pair where the first element is a boolean indicating whether the character was found, and the second element
     *         is the position of the character in bytes from the start of the container (or std::numeric_limits<size_t>::max() if not found).
     */
    virtual std::optional<size_t> findChar(const int & c, const size_t &offset = 0, size_t searchSpace = 0, bool caseSensitive = false) = 0;

    /**
     * @brief find memory into the container
     * @param needle memory data to be found.
     * @param len memory data to be found size in bytes.
     * @param offset container offset where to start to find.
     * @param searchSpace search space size in bytes where is going to find the needle. (zero for all the space)
     * @return position of the needle (if found)
     */
    std::optional<size_t> find(const void * needle, const size_t &needle_len, bool caseSensitive = true, const size_t &offset = 0, size_t searchSpace = 0);
    /**
     * @brief find memory into the container
     * @param needle memory data to be found.
     * @param len memory data to be found size in bytes.
     * @param offset container offset where to start to find.
     * @param searchSpace search space size in bytes where is going to find the needle. (zero for all the space)
     * @return position of the needle (if found)
     */
    std::optional<size_t> find(const std::list<std::string> &needles, std::string & needleFound, bool caseSensitive = true, const size_t &offset = 0, const size_t &searchSpace = 0);

    // Data Size:
    /**
     * @brief size Get Container Data Size in bytes
     * @return data size in bytes
     */
    virtual size_t size() override;
    /**
     * @brief Is the container Null
     * @return true if container size is 0.
     */
    bool isNull();
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Container behaviour.
    /**
     * @brief Get Maximum Container Size allowed in bytes
     * @return Maximum Container Size allowed in bytes
     */
    size_t getMaxSize() const;
    /**
     * @brief Set Maximum Container Size allowed in bytes
     * @param value Maximum Container Size allowed in bytes
     */
    void setMaxSize(const size_t &value);
    /**
     * @brief getSizeLeft Get maximum appendable amount of data.
     * @return maximum appendable ammount of data.
     */
    size_t getSizeLeft();

    /**
     * @brief reduceMaxSizeBy Reduce the maximum size by value
     * @param value value to reduce
     */
    void reduceMaxSizeBy(const size_t &value);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Streamable
    bool streamTo(Memory::Streams::StreamableObject *out) override;
    std::optional<size_t> write(const void *buf, const size_t &count) override;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // FS options:
    std::shared_ptr<B_Base> copyToFS(const std::string &fileName, bool deleteFileOnDestruction);

    /**
     * @brief get the directory path where the temporary file will be saved
     * @return directory path
     */
    std::string getFsDirectoryPath() const;
    /**
     * @brief set the directory path where the temporary file will be saved
     * @param value directory path (eg. /tmp)
     */
    void setFsDirectoryPath(const std::string &value);
    /**
     * @brief getFsBaseFileName Get base file name used for creating temporary files.
     * @return Base file name string
     */
    std::string getFsBaseFileName() const;
    /**
     * @brief setFsBaseFileName Set base file name used for creating temporary files.
     * @param value Base file name string to be set
     */
    void setFsBaseFileName(const std::string &value);
    /**
     * @brief getCurrentFileName Get the current full path of the temporary file.
     * @return Full path string of the current file
     */
    virtual std::string getCurrentFileName() const;


protected:
    /**
     * @brief truncate the current container to n bytes.
     * @param bytes n bytes.
     * @return new container size.
     */
    virtual std::optional<size_t> truncate2(const size_t &bytes) = 0;

    /**
     * @brief free the whole container
     * @return true if succeed
     */
    virtual bool clear2() = 0;
    /**
     * @brief remove n bytes at the beggining shrinking the container
     * @param bytes bytes to be removed
     * @return bytes removed.
     */
    virtual std::optional<size_t> displace2(const size_t &bytes = 0) = 0;
    /**
     * @brief Append more data to current chunks. (creates new chunks of data)
     * @param buf data to be appended
     * @param len data size in bytes to be appended
     * @param prependMode mode: true will prepend the data, false will append.
     * @return pair containing a boolean indicating success and the number of bytes processed.
     */
    virtual std::optional<size_t> append2(const void * buf, const size_t &len, bool prependMode) = 0;
    
    /**
    * @brief Internal Copy function to copy this container to a stream
    * @param out data stream out
     * @param bytes size of data to be copied in bytes. -1 (default) copies all the container but the offset.
    * @param offset displacement in bytes where the data starts.
     * @return pair containing a boolean indicating success and the number of bytes processed.
    */
    virtual std::optional<size_t> copyToStream2(std::ostream & out, const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0) = 0;

    /**
     * @brief Internal Copy function to copy this container to a new one with status update.
     * @param bc destination StreamableObject
     * @param bytes size of data to be copied in bytes. -1 (default) copies all the container but the offset.
     * @param offset displacement in bytes where the data starts.
     * @return pair containing a boolean indicating success and the number of bytes processed.
     */
    virtual std::optional<size_t> copyToStreamableObject2(StreamableObject & bc, const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0) = 0;
    /**
     * @brief Copy data from the container to an external buffer
     * @param buf Destination buffer where data will be copied
     * @param bytes Number of bytes to copy
     * @param offset Starting point (offset) in bytes, default is 0 (start)
     * @return Pair containing a boolean indicating success and the number of bytes copied
     */
    virtual std::optional<size_t> copyToBuffer2(void * buf, const size_t &bytes, const size_t &offset = 0) = 0;
    /**
     * @brief Compare data in the container with an external memory buffer
     * @param buf Memory buffer to compare against
     * @param len Size of the memory buffer in bytes
     * @param caseSensitive Flag indicating whether comparison should be case-sensitive, default is true
     * @param offset Starting point (offset) in bytes for comparison, default is 0 (start)
     * @return Boolean indicating whether the comparison was successful and the contents were equal
     */
    virtual bool compare2(const void * buf, const size_t &len, bool caseSensitive = true, const size_t &offset = 0 ) = 0;

    /**
     * @brief incContainerBytesCount Internal function to increase container bytes count.
     * @param i Number of bytes to add to the container's byte count
     */
    void incContainerBytesCount(const size_t & i);

    /**
     * @brief decContainerBytesCount Internal function to decrease container bytes count.
     * @param i Number of bytes to subtract from the container's byte count
     */
    void decContainerBytesCount(const size_t & i);

    // Auxiliar:
    std::optional<size_t> copyToStreamUsingCleanVector(std::ostream &streamOut, std::vector<BinaryContainerChunk> copyChunks);
    std::optional<size_t> copyToStreamableObjectUsingCleanVector(StreamableObject &bcOut, std::vector<BinaryContainerChunk> copyChunks);

    void setContainerBytes(const size_t &value);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // Variables::


    // FS directives:
    /**
     * @brief fsDirectoryPath Directory where files will be created.
     */
    std::string m_fsDirectoryPath;
    /**
     * @brief fsBaseFileName base filename on the directory.
     */
    std::string m_fsBaseFileName;

    // Storage Method:
    /**
     * @brief storeMethod Storage Mechanism used (read only memory reference, chunks, file)
     */
    BinaryContainerMethod m_storeMethod;

    /**
     * @brief readOnly defined if it's in read-only mode or not.
     */
    bool m_readOnly;

    /**
     * @brief containerBytes container current size in bytes.
     */
    size_t m_containerBytes;
    /**
     * @brief maxSize Maximum size of the container
     */
    size_t m_maxSize;

private:
    bool clear0();

};

}}}

