#include "a_var.h"

#include "a_bin.h"
#include "a_bool.h"
#include "a_datetime.h"
#include "a_double.h"
#include "a_int16.h"
#include "a_int32.h"
#include "a_int64.h"
#include "a_int8.h"
#include "a_ipv4.h"
#include "a_ipv6.h"
#include "a_macaddr.h"
#include "a_string.h"
#include "a_stringlist.h"
#include "a_uint16.h"
#include "a_uint32.h"
#include "a_uint64.h"
#include "a_uint8.h"
#include "a_varchar.h"
#include <memory>

using namespace Mantids30::Memory::Abstract;

Var::Var()
{
    m_varType = Type::VOID;
}

std::shared_ptr<Var> Var::copy()
{
    std::shared_ptr<Var> var = protectedCopy();
    if (var)
    {
        var->setVarType(this->getVarType());
    }
    return var;
}

void *Var::getDirectMemory()
{
    return nullptr;
}

std::shared_ptr<Var> Var::makeAbstract(Var::Type type, const std::string &defValue)
{
    std::shared_ptr<Var> v = nullptr;
    switch (type)
    {
    case Var::Type::VOID:
        v = std::make_shared<Var>();
        break;
    case Var::Type::BOOL:
        v = std::make_shared<BOOL>();
        break;
    case Var::Type::INT8:
        v = std::make_shared<INT8>();
        break;
    case Var::Type::INT16:
        v = std::make_shared<INT16>();
        break;
    case Var::Type::INT32:
        v = std::make_shared<INT32>();
        break;
    case Var::Type::INT64:
        v = std::make_shared<INT64>();
        break;
    case Var::Type::UINT8:
        v = std::make_shared<UINT8>();
        break;
    case Var::Type::UINT16:
        v = std::make_shared<UINT16>();
        break;
    case Var::Type::UINT32:
        v = std::make_shared<UINT32>();
        break;
    case Var::Type::UINT64:
        v = std::make_shared<UINT64>();
        break;
    case Var::Type::DOUBLE:
        v = std::make_shared<DOUBLE>();
        break;
    case Var::Type::DATETIME:
        v = std::make_shared<DATETIME>();
        break;
    case Var::Type::BIN:
        v = std::make_shared<BINARY>();
        break;
    case Var::Type::STRING:
        v = std::make_shared<STRING>();
        break;
    case Var::Type::STRINGLIST:
        v = std::make_shared<STRINGLIST>();
        break;
    case Var::Type::IPV4:
        v = std::make_shared<IPV4>();
        break;
    case Var::Type::MACADDR:
        v = std::make_shared<MACADDR>();
        break;
    case Var::Type::IPV6:
        v = std::make_shared<IPV6>();
        break;
    case Var::Type::VARCHAR:
        v = std::make_shared<VARCHAR>(defValue.size() + 1024);
        break;
    default:
        break;
    }

    if (v)
    {
        v->fromString(defValue);
    }

    return v;
}

Json::Value Var::toJSON()
{
    return Json::Value::null;
}

bool Var::fromJSON(const Json::Value &value)
{
    return true;
}

std::string Var::toString()
{
    return "";
}

bool Var::fromString(const std::string &)
{
    return true;
}

Var::Type Var::getVarType() const
{
    return m_varType;
}

void Var::setVarType(const Type &value)
{
    m_varType = value;
    this->m_isNull = false;
}

std::shared_ptr<Var> Var::protectedCopy()
{
    std::shared_ptr<Var> var = std::make_shared<Var>();
    return var;
}
bool Var::isNull()
{
    return m_isNull;
}

void Var::setIsNull(bool newIsNull)
{
    m_isNull = newIsNull;
}
