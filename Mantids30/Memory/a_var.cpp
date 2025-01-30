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

using namespace Mantids30::Memory::Abstract;

Var::Var()
{
    varType = TYPE_NULL;
}

Var *Var::copy()
{
    Var *var = protectedCopy();
    if (var)
        var->setVarType(this->getVarType());
    return var;
}

Var::~Var() {}

void *Var::getDirectMemory()
{
    return nullptr;
}

std::shared_ptr<Var> Var::makeAbstract(Var::Type type, const std::string &defValue)
{
    std::shared_ptr<Var> v = nullptr;
    switch (type)
    {
    case Var::TYPE_NULL:
        v = std::make_shared<Var>();
        break;
    case Var::TYPE_BOOL:
        v = std::make_shared<BOOL>();
        break;
    case Var::TYPE_INT8:
        v = std::make_shared<INT8>();
        break;
    case Var::TYPE_INT16:
        v = std::make_shared<INT16>();
        break;
    case Var::TYPE_INT32:
        v = std::make_shared<INT32>();
        break;
    case Var::TYPE_INT64:
        v = std::make_shared<INT64>();
        break;
    case Var::TYPE_UINT8:
        v = std::make_shared<UINT8>();
        break;
    case Var::TYPE_UINT16:
        v = std::make_shared<UINT16>();
        break;
    case Var::TYPE_UINT32:
        v = std::make_shared<UINT32>();
        break;
    case Var::TYPE_UINT64:
        v = std::make_shared<UINT64>();
        break;
    case Var::TYPE_DOUBLE:
        v = std::make_shared<DOUBLE>();
        break;
    case Var::TYPE_DATETIME:
        v = std::make_shared<DATETIME>();
        break;
    case Var::TYPE_BIN:
        v = std::make_shared<BINARY>();
        break;
    case Var::TYPE_STRING:
        v = std::make_shared<STRING>();
        break;
    case Var::TYPE_STRINGLIST:
        v = std::make_shared<STRINGLIST>();
        break;
    case Var::TYPE_IPV4:
        v = std::make_shared<IPV4>();
        break;
    case Var::TYPE_MACADDR:
        v = std::make_shared<MACADDR>();
        break;
    case Var::TYPE_IPV6:
        v = std::make_shared<IPV6>();
        break;
    case Var::TYPE_VARCHAR:
        v = std::make_shared<VARCHAR>(defValue.size() + 1024);
        break;
    default:
        break;
    }

    if (v)
        v->fromString(defValue);

    return v;
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
    return varType;
}

void Var::setVarType(const Type &value)
{
    varType = value;
}

Var *Var::protectedCopy()
{
    Var *var = new Var;
    return var;
}
