#include "a_stringlist.h"
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Memory::Abstract;

STRINGLIST::STRINGLIST()
{
    setVarType(TYPE_STRINGLIST);
}

STRINGLIST::STRINGLIST(const std::list<std::string> &value)
{
    setVarType(TYPE_STRINGLIST);
    this->value = value;
}


std::list<std::string> STRINGLIST::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

bool STRINGLIST::setValue(const std::list<std::string> &value)
{
    Threads::Sync::Lock_RW lock(mutex);
    this->value = value;
    return true;
}

std::string STRINGLIST::toString()
{
    std::list<std::string> xvalue = getValue();
    // TODO:  use "" and escape seq CSV format.
    std::string r;
    bool first = true;
    for (const std::string & element : xvalue)
    {
        r += (!first? "," : "") + element;
        if (first) first = false;
    }
    return r;
}

bool STRINGLIST::fromString(const std::string &value)
{
    // TODO:  use "" and escape seq CSV format.
    std::list<std::string> strs;
    std::string::size_type curpos = 0, pos = std::string::npos;
    while (1)
    {
        pos = value.find_first_of(',',curpos);
        // Last word....
        std::string svalue;
        if (pos != std::string::npos)
        {
            svalue = value.substr(curpos, pos-curpos);
            strs.push_back(svalue);
            curpos=pos+1;
        }
        else
        {
            svalue = value.substr(curpos, std::string::npos);
            strs.push_back(svalue);
            break;
        }
    }
    return setValue(strs);
}

Var *STRINGLIST::protectedCopy()
{
    STRINGLIST * var = new STRINGLIST;
    if (var) *var = getValue();
    return var;
}
