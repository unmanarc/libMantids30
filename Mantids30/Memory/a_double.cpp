#include "a_double.h"
#include <Mantids30/Threads/lock_shared.h>

#include <stdexcept>      // std::invalid_argument

using namespace Mantids30::Memory::Abstract;

DOUBLE::DOUBLE()
{
    setVarType(TYPE_DOUBLE);
}

DOUBLE::DOUBLE(const double &value)
{
    setVarType(TYPE_DOUBLE);
    this->m_value = value;
}

double DOUBLE::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return m_value;
}

void DOUBLE::setValue(const double &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    this->m_value = value;
}

std::string DOUBLE::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return std::to_string(m_value);
}

bool DOUBLE::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    try
    {
        this->m_value = std::stod( value ) ;
        return true;
    }
    catch( std::invalid_argument * )
    {
        return false;
    }
    catch ( std::out_of_range * )
    {
        return false;
    }
}
std::shared_ptr<Var> DOUBLE::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<DOUBLE>();
    if (var) *var = this->m_value;
    return var;
}
