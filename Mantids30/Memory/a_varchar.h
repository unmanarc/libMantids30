#pragma once


#include "a_var.h"
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 { namespace Memory { namespace Abstract {

class VARCHAR : public Var
{
public:
    VARCHAR(const size_t & varSize);
    VARCHAR(const size_t & varSize, char * value);
    VARCHAR(VARCHAR &var);
    virtual ~VARCHAR() override;

    /**
     * @brief operator =  Copy Value from a null terminated string (check with getWasTruncated)
     * @param value null terminated string.
     * @return current class.
     */
    VARCHAR& operator=(char * value)
    {
        setValue(value);
        return *this;
    }

    std::string toString() override;
    bool fromString(const std::string & value) override;

    /**
     * @brief getValue Get the null terminated string (can be reused)
     * @return null terminated string.
     */
    char *getValue();
    /**
     * @brief setValue Copy Value from a null terminated string
     * @param value null terminated string
     * @return true if sucessfully copied, false if it was truncated.
     */
    bool setValue(char *value);

    size_t getVarSize();

    void * getDirectMemory() override { return &m_value; }

    /**
     * @brief getWasTruncated Get if the last copy operation was truncated.
     * @return true if truncated.
     */
    bool getWasTruncated();

    unsigned long getFillSize() const;
    unsigned long * getFillSizePTR() { return &m_fillSize; }

protected:
    std::shared_ptr<Var> protectedCopy() override;
private:
    bool m_wasTruncated;
    char * m_value;
    size_t m_varSize;
    unsigned long m_fillSize;
    Threads::Sync::Mutex_Shared m_mutex;

};

}}}

