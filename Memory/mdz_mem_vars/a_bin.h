#ifndef A_BIN_H
#define A_BIN_H

#include "a_var.h"
#include <mdz_thr_mutex/mutex_shared.h>
#include <string.h>

namespace Mantids { namespace Memory { namespace Abstract {

struct sBinContainer {
    sBinContainer()
    {
        ptr = nullptr;
        dataSize = 0;
    }
    ~sBinContainer()
    {
        if (ptr) delete [] ptr;
    }
    sBinContainer(const size_t & len)
    {
        ptr = nullptr;
        if (len==0) return;
        ptr = new char[len+1];
        if (!ptr) return;
        memset(ptr,0,len+1);
        dataSize = len;
    }
    sBinContainer(char * value, const size_t & len)
    {
        ptr = nullptr;
        if (len==0) return;
        ptr = new char[len+1];
        if (!ptr) return;
        ptr[len] = 0;
        dataSize = len;
        memcpy(ptr,value,len);
    }
    char * ptr;
    size_t dataSize;
    Threads::Sync::Mutex_Shared mutex;
};

class BINARY : public Var
{
public:
    BINARY();
    virtual ~BINARY() override;

    /**
     * @brief getValue Get container memory position
     * @return container memory position and mutex locked (you have to unlock it).
     */
    sBinContainer *getValue();
    bool setValue(sBinContainer *value);

    std::string toString() override;
    bool fromString(const std::string & value) override;

    void * getDirectMemory() override { return &value; }

protected:
    Var * protectedCopy() override;

private:
    sBinContainer value;

};
}}}

#endif // A_BIN_H
