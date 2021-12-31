#ifndef ABSTRACTVAR_H
#define ABSTRACTVAR_H

#include <string>

namespace Mantids { namespace Memory { namespace Abstract {

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

#define ABSTRACT_PTR_AS(x,y) ((Mantids::Memory::Abstract::x *)y)

class Var
{
public:
    Var();
    Var * copy();
    virtual ~Var();

    // NON-THREADSAFE ACCESS TO THE RAW MEMORY:
    virtual void * getDirectMemory();

    static Var * makeAbstract(Type type, const std::string & defValue = "");

    /**
     * @brief toString Transform the Variable To a Readable string.
     * @return readable string
     */
    virtual std::string toString();
    /**
     * @brief fromString Set the Variable value from a readable string.
     * @param value readable string
     * @return true if accepted.
     */
    virtual bool fromString(const std::string & value);

    // VAR TYPE:
    Type getVarType() const;
    void setVarType(const Type &value);

protected:
    virtual Var * protectedCopy();

private:
    Type varType;
};

}}}



#endif // ABSTRACTVAR_H
