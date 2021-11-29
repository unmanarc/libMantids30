#include "a_var.h"

#include "a_bin.h"
#include "a_bool.h"
#include "a_double.h"
#include "a_int8.h"
#include "a_int16.h"
#include "a_int32.h"
#include "a_int64.h"
#include "a_ipv4.h"
#include "a_macaddr.h"
#include "a_ipv6.h"
#include "a_string.h"
#include "a_stringlist.h"
#include "a_uint8.h"
#include "a_uint16.h"
#include "a_uint32.h"
#include "a_uint64.h"
#include "a_varchar.h"
#include "a_datetime.h"

using namespace CX2::Memory::Abstract;

Var::Var()
{
    varType = TYPE_NULL;
}

Var *Var::copy()
{
    Var * var = protectedCopy();
    if (var) var->setVarType(this->getVarType());
    return var;
}

Var::~Var()
{

}

void *Var::getDirectMemory()
{
    return nullptr;
}

Var *Var::makeAbstract(Type type, const std::string &defValue)
{
    Var * v = nullptr;
    switch (type)
    {
    case TYPE_NULL:
        v=new Var;
        break;
    case TYPE_BOOL:
        v=new BOOL;
        break;
    case TYPE_INT8:
        v=new INT8;
        break;
    case TYPE_INT16:
        v=new INT16;
        break;
    case TYPE_INT32:
        v=new INT32;
        break;
    case TYPE_INT64:
        v=new INT64;
        break;
    case TYPE_UINT8:
        v=new UINT8;
        break;
    case TYPE_UINT16:
        v=new UINT16;
        break;
    case TYPE_UINT32:
        v=new UINT32;
        break;
    case TYPE_UINT64:
        v=new UINT64;
        break;
    case TYPE_DOUBLE:
        v=new DOUBLE;
        break;
    case TYPE_DATETIME:
        v=new DATETIME;
        break;
    case TYPE_BIN:
        v=new BINARY;
        break;
    case TYPE_STRING:
        v=new STRING;
        break;
    case TYPE_STRINGLIST:
        v=new STRINGLIST;
        break;
    case TYPE_IPV4:
        v=new IPV4;
        break;
    case TYPE_MACADDR:
        v=new MACADDR;
        break;
    case TYPE_IPV6:
        v=new IPV6;
        break;
    case TYPE_VARCHAR:
        v=new VARCHAR(defValue.size()+1024);
        break;
    default:
        break;
    }

    if (v) v->fromString(defValue);

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

Type Var::getVarType() const
{
    return varType;
}

void Var::setVarType(const Type &value)
{
    varType = value;
}

Var *Var::protectedCopy()
{
    Var * var = new Var;
    return var;
}
