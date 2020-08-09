#include "abstract.h"

#include "a_bin.h"
#include "a_bool.h"
#include "a_double.h"
#include "a_int8.h"
#include "a_int16.h"
#include "a_int32.h"
#include "a_int64.h"
#include "a_ipv4.h"
#include "a_ipv6.h"
#include "a_string.h"
#include "a_stringlist.h"
#include "a_uint8.h"
#include "a_uint16.h"
#include "a_uint32.h"
#include "a_uint64.h"

using namespace CX2::Memory::Vars;

Abstract::Abstract()
{

}

Abstract *Abstract::copy()
{
    Abstract * var = protectedCopy();
    if (var) var->setVarType(this->getVarType());
    return var;
}

Abstract::~Abstract()
{

}

Abstract *Abstract::makeAbstract(AVarType type, const std::string &defValue)
{
    Abstract * v = nullptr;
    switch (type)
    {
    case ABSTRACT_BOOL:
        v=new A_BOOL;
        break;
    case ABSTRACT_INT8:
        v=new A_INT8;
        break;
    case ABSTRACT_INT16:
        v=new A_INT16;
        break;
    case ABSTRACT_INT32:
        v=new A_INT32;
        break;
    case ABSTRACT_INT64:
        v=new A_INT64;
        break;
    case ABSTRACT_UINT8:
        v=new A_UINT8;
        break;
    case ABSTRACT_UINT16:
        v=new A_UINT16;
        break;
    case ABSTRACT_UINT32:
        v=new A_UINT32;
        break;
    case ABSTRACT_UINT64:
        v=new A_UINT64;
        break;
    case ABSTRACT_DOUBLE:
        v=new A_DOUBLE;
        break;
    case ABSTRACT_BIN:
        v=new A_BIN;
        break;
    case ABSTRACT_STRING:
        v=new A_STRING;
        break;
    case ABSTRACT_STRINGLIST:
        v=new A_STRINGLIST;
        break;
    case ABSTRACT_IPV4:
        v=new A_IPV4;
        break;
    case ABSTRACT_IPV6:
        v=new A_IPV6;
        break;
    default:
        break;
    }

    if (v) v->fromString(defValue);

    return v;
}

AVarType Abstract::getVarType() const
{
    return varType;
}

void Abstract::setVarType(const AVarType &value)
{
    varType = value;
}
