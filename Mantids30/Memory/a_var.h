#pragma once

#include <memory>
#include <string>
#include <atomic>

#include <Mantids30/Helpers/json.h>

namespace Mantids30::Memory::Abstract {

#define ABSTRACT_PTR_AS(x,y) ( static_cast<Mantids30::Memory::Abstract::x *>(y))
#define ABSTRACT_SPTR_AS(x,y) (std::dynamic_pointer_cast<Mantids30::Memory::Abstract::x>(y))
#define MAKE_VAR(x,y)  std::make_shared<Mantids30::Memory::Abstract::x>(y)
#define MAKE_NULL_VAR  std::make_shared<Mantids30::Memory::Abstract::Var>()

class Var
{
public:
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

    Var();
    std::shared_ptr<Var> copy();
    virtual ~Var() = default;

    // NON-THREADSAFE ACCESS TO THE RAW MEMORY:
    virtual void * getDirectMemory();

    static std::shared_ptr<Var> makeAbstract(Type type, const std::string & defValue = "");

    virtual json toJSON();;
    virtual bool fromJSON(const json & value);

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

    bool isNull();
    void setIsNull(bool newIsNull);

protected:
    virtual std::shared_ptr<Var> protectedCopy();
    std::atomic<bool> m_isNull = true;

private:

    Type m_varType;
};

}



