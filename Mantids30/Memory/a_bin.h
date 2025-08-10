#pragma once

#include "a_var.h"
#include <Mantids30/Threads/mutex_shared.h>
#include <Mantids30/Helpers/encoders.h>
#include <string.h>

namespace Mantids30 { namespace Memory { namespace Abstract {


class BINARY : public Var
{
public:
    struct sBinContainer {
        sBinContainer()
        {
            ptr = nullptr;
            dataSize = 0;
        }
        ~sBinContainer()
        {
            if (ptr)
                delete [] ptr;
        }
        sBinContainer(const size_t & len)
        {
            ptr = nullptr;
            if (len==0) 
                return;
            ptr = new char[len+1];
            if (!ptr) 
                return;
            memset(ptr,0,len+1);
            dataSize = len;
        }
        sBinContainer(char * value, const size_t & len)
        {
            ptr = nullptr;
            if (len==0) 
                return;
            ptr = new char[len+1];
            if (!ptr) 
                return;
            ptr[len] = 0;
            dataSize = len;
            memcpy(ptr,value,len);
        }
        char * ptr;
        size_t dataSize;
        Threads::Sync::Mutex_Shared mutex;
    };

    BINARY();
    virtual ~BINARY() override = default;

    /**
     * @brief getValue Get container memory position
     * @return container memory position and mutex locked (you have to unlock it).
     */
    sBinContainer *getValue();
    bool setValue(sBinContainer *value);

    std::string toString() override;
    bool fromString(const std::string & value) override;

    json toJSON() override;
    bool fromJSON(const json &value) override;


    void * getDirectMemory() override { return &m_value; }

protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    sBinContainer m_value;

};
}}}

