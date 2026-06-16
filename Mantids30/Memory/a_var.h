#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <Mantids30/Helpers/json.h>

namespace Mantids30::Memory::Abstract {

#define ABSTRACT_PTR_AS(x, y) (static_cast<Mantids30::Memory::Abstract::x *>(y))
#define ABSTRACT_SPTR_AS(x, y) (std::dynamic_pointer_cast<Mantids30::Memory::Abstract::x>(y))
#define MAKE_VAR(x, y) std::make_shared<Mantids30::Memory::Abstract::x>(y)
#define MAKE_NULL_VAR std::make_shared<Mantids30::Memory::Abstract::Var>()

class Var
{
public:
    enum class Type : uint8_t
    {
        BOOL,
        INT8,
        INT16,
        INT32,
        INT64,
        UINT8,
        UINT16,
        UINT32,
        UINT64,
        DOUBLE,
        BIN,
        STRING,
        VARCHAR,
        STRINGLIST,
        IPV4,
        IPV6,
        MACADDR,
        PTR,
        DATETIME,
        VOID
    };

    Var();
    [[nodiscard]] std::shared_ptr<Var> copy();
    virtual ~Var() = default;

    // NON-THREADSAFE ACCESS TO THE RAW MEMORY:
    [[nodiscard]] virtual void *getDirectMemory();

    [[nodiscard]] static std::shared_ptr<Var> makeAbstract(Type type, const std::string &defValue = "");

    [[nodiscard]] virtual json toJSON();
    [[nodiscard]] virtual bool fromJSON(const json &value);

    /**
     * @brief toString Transform the Variable To a Readable string.
     * @return readable string
     */
    [[nodiscard]] virtual std::string toString();
    /**
     * @brief fromString Set the Variable value from a readable string.
     * @param value readable string
     * @return true if accepted.
     */
    virtual bool fromString(const std::string &value);

    // VAR TYPE:
    [[nodiscard]] Type getVarType() const;
    void setVarType(const Type &value);

    [[nodiscard]] bool isNull();
    void setIsNull(bool newIsNull);

protected:
    virtual std::shared_ptr<Var> protectedCopy();
    std::atomic<bool> m_isNull = true;

private:
    Type m_varType;
};

} // namespace Mantids30::Memory::Abstract
