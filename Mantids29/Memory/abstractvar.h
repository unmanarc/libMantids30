#ifndef ABSTRACTVAR_H
#define ABSTRACTVAR_H

#include <stdint.h>
#include <string>
#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Memory { namespace Abstract {

class Base {
public:
    Base() { };
    enum Type {
        TYPE_BOOL,
        TYPE_INT8,
        TYPE_INT16,
        TYPE_INT32,
        TYPE_INT64,
        TYPE_UINT8,
        TYPE_UINT16,
        TYPE_UINT32,
        TYPE_UINT64,
        TYPE_DOUBLE,
        TYPE_BIN,
        TYPE_STRING,
        TYPE_VARCHAR,
        TYPE_STRINGLIST,
        TYPE_IPV4,
        TYPE_IPV6,
        TYPE_MACADDR,
        TYPE_PTR,
        TYPE_DATETIME,
        TYPE_NULL
    };

    Base *copy();
    /**
     * @brief toString Transform the Variable To a Readable string.
     * @return readable string
     */
    virtual std::string toString() { return ""; }
    /**
     * @brief fromString Set the Variable value from a readable string.
     * @param value readable string
     * @return true if accepted.
     */
    virtual bool fromString(const std::string & value) { return true; }

    Base *makeAbstract(Type type, const std::string &defValue);

    Type varType = {TYPE_NULL};

protected:
    virtual Base * protectedCopy();

};


template <typename T>
class AbstractVar : public Base
{
public:
    AbstractVar();
    AbstractVar(const T &value) : value(value) {}

    AbstractVar& operator=(const T & value)
    {
        setValue(value);
        return *this;
    }

    T getValue() { return value; }
    bool setValue(const T & value);

    void * getDirectMemory();

    std::string toString();

    bool fromString(const std::string & value);

protected:
    AbstractVar * protectedCopy()
    {
        return new AbstractVar<T>(*this);
    }

private:
    T value;
    Threads::Sync::Mutex_Shared mutex;
};


using INT32 = AbstractVar<int32_t>;
using UINT64 = AbstractVar<uint64_t>;

}}}
#endif // ABSTRACTVAR_H
