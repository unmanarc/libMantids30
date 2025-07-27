#pragma once

#include "b_base.h"

namespace Mantids30 { namespace Memory { namespace Containers {


class B_Ref : public B_Base
{
public:
    B_Ref(B_Base *bc = nullptr, const size_t &offset=0, const size_t &maxBytes=0);
    ~B_Ref() override;
    /**
     * @brief reference another binary container into this container.
     * @param bc binary container to be referenced.
     * @param offset start position.
     * @param maxBytes max bytes to be referenced (std::numeric_limits<size_t>::max(): unlimited, but if maxBytes>0, it will be readOnly)
     */
    bool reference(B_Base * bc, const size_t &offset=0, const size_t &maxBytes = std::numeric_limits<size_t>::max());
    /**
     * @brief getReferencedBC Get referenced object
     * @return referenced object pointer.
     */
    B_Base *getReferencedBC() const;
    /**
     * @brief size Get Container Data Size in bytes
     * @return data size in bytes
     */
    size_t size() override;
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
     * @brief Append is disabled.
     * @return 0.
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
    * @param bc Binary container.
    * @param bytes size of data to be copied in bytes. -1 copy all the container but the offset.
    * @param offset displacement in bytes where the data starts.
    * @return
    */
    std::optional<size_t> copyToStream2(std::ostream & out, const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0) override;
    /**
    * @brief Internal Copy function to copy this container to a new one.
    * @param out data stream out
    * @param bytes size of data to be copied in bytes. -1 copy all the container but the offset.
    * @param offset displacement in bytes where the data starts.
    * @return
    */
    std::optional<size_t> copyToStreamableObject2(StreamableObject &bc, const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0) override;
    /**
     * @brief Copy append to another binary container.
     * @param bc destination binary container
     * @param bytes size of data in bytes to be copied
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return number of bytes copied (in bytes)
     */
    std::optional<size_t> copyToBuffer2(void * buf, const size_t &bytes, const size_t &offset = 0) override;
    /**
     * @brief Compare memory with the container
     * @param mem Memory to be compared
     * @param len Memory size in bytes to be compared
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return true where comparison returns equeal.
     */
    bool compare2(const void * buf, const size_t &len, bool caseSensitive = true, const size_t &offset = 0 ) override;


private:
    /**
     * @brief referencedBC referenced Binary container (then, this container will not work autonomous)
     */
    B_Base * referencedBC;
    /**
     * @brief referecedOffset binary container reference offset in bytes
     */
    size_t referencedOffset;
    /**
     * @brief referencedMaxBytes binary container reference bytes to be referenced
     */
    size_t referencedMaxBytes;

};

}}}

