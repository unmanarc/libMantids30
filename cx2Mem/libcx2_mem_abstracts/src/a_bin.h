#ifndef ABSVAR_BIN_H
#define ABSVAR_BIN_H

#include "abstract.h"
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Vars {

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
    uint32_t dataSize;
    Threads::Sync::Mutex_Shared mutex;
};

class A_BIN : public Abstract
{
public:
    A_BIN();
    virtual ~A_BIN() override;

    /**
     * @brief getValue Get container memory position
     * @return container memory position and mutex locked (you have to unlock it).
     */
    sBinContainer *getValue();
    bool setValue(sBinContainer *value);

    std::string toString() override;
    bool fromString(const std::string & value) override;

protected:
    Abstract * protectedCopy() override;

private:
    sBinContainer value;
};
}}}

#endif // ABSVAR_BIN_H
