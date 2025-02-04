#pragma once

#include "b_base.h"

namespace Mantids30 { namespace Memory { namespace Containers {


class B_MEM : public B_Base
{
public:
    B_MEM(const void * buf=nullptr, const uint32_t &len=0);
    ~B_MEM() override;
    void reference(const void * buf, const uint32_t & len);
    
    /**
    * @brief Searches for the first occurrence of a character within a specified range and offset.
    *
    * This function searches for the first occurrence of a given character (charInt) within
    * a specified range (searchSpace) starting from a specific offset in the memory buffer.
    * It supports case-sensitive or case-insensitive search.
    *
    * @param charInt The ASCII value of the character to search for.
    * @param offset The starting position in the memory buffer to begin searching.
    * @param searchSpace The number of bytes to search within, starting from the specified offset.
    *                    If set to 0, the function will search until the end of the buffer.
    * @param caseSensitive A boolean flag indicating whether the search should be case-sensitive.
    *
    * @return A pair consisting of a boolean and an unsigned integer. The boolean indicates whether
    *         the character was found (true if found, false otherwise). The unsigned integer represents
    *         the offset from the beginning of the buffer to the first occurrence of the character,
    *         or 0 if the character is not found.
    *
    * @note If caseSensitive is true but the character is not a letter (A-Z or a-z), the search will be
    *       treated as case-insensitive.
    */
    std::pair<bool,uint64_t> findChar(const int & c, const uint64_t &offset = 0, uint64_t searchSpace = 0, bool caseSensitive = false) override;

protected:
    /**
     * @brief truncate the current container to n bytes.
     * @param bytes n bytes.
     * @return new container size.
     */
    std::pair<bool, uint64_t> truncate2(const uint64_t &bytes) override;
    /**
     * @brief Append is disabled.
     * @return 0.
     */
    std::pair<bool,uint64_t> append2(const void * buf, const uint64_t &len, bool prependMode = false) override;
    /**
     * @brief remove n bytes at the beggining shrinking the container
     * @param bytes bytes to be removed
     * @return bytes removed.
     */
    std::pair<bool, uint64_t> displace2(const uint64_t &bytes = 0) override;
    /**
     * @brief free the whole container
     * @return true if succeed
     */
    bool clear2() override;
    /**
    * @brief Append this current container to a new one.
    * @param bc Binary container.
    * @param bytes size of data to be copied in bytes. -1 copy all the container but the offset.
    * @param offset displacement in bytes where the data starts.
    * @return
    */
    std::pair<bool, uint64_t> copyToStream2(std::ostream & bc, const uint64_t &bytes = std::numeric_limits<uint64_t>::max(), const uint64_t &offset = 0) override;
    /**
    * @brief Internal Copy function to copy this container to a new one.
    * @param out data stream out
    * @param bytes size of data to be copied in bytes. -1 copy all the container but the offset.
    * @param offset displacement in bytes where the data starts.
    * @return
    */
    std::pair<bool,uint64_t> copyTo2(StreamableObject & bc, Streams::StreamableObject::Status &wrStatUpd, const uint64_t &bytes = std::numeric_limits<uint64_t>::max(), const uint64_t &offset = 0) override;
    /**
     * @brief Copy append to another binary container.
     * @param bc destination binary container
     * @param bytes size of data in bytes to be copied
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return number of bytes copied (in bytes)
     */
    std::pair<bool, uint64_t> copyOut2(void * buf, const uint64_t &bytes, const uint64_t &offset = 0) override;
    /**
     * @brief Compare memory with the container
     * @param mem Memory to be compared
     * @param len Memory size in bytes to be compared
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return true where comparison returns equeal.
     */
    bool compare2(const void * buf, const uint64_t &len, bool caseSensitive = true, const uint64_t &offset = 0 ) override;


private:
    // Linear Memory:
    /**
     * @brief linearMem Current usable linear memory pointer.
     */
    const char * linearMem = nullptr;
};

}}}

