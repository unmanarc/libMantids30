#ifndef A_BIN_H
#define A_BIN_H

#include "a_var.h"
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

struct sBinContainer {
    sBinContainer()
    {
        ptr = nullptr;
    }
    ~sBinContainer()
    {
        if (ptr) delete [] ptr;
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
