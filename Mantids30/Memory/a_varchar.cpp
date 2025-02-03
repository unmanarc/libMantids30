#include "a_varchar.h"
#include <string.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

VARCHAR::VARCHAR(const size_t &varSize)
{
    setVarType(TYPE_VARCHAR);
    m_wasTruncated = false;
    this->m_varSize = varSize;
    this->m_value = (char *)malloc(varSize+1);
    this->m_value[varSize] = 0;
}

VARCHAR::VARCHAR(const size_t &varSize, char *value)
{
    setVarType(TYPE_VARCHAR);
    m_wasTruncated = false;
    this->m_varSize = varSize;
    this->m_value = (char *)malloc(varSize+1);
    this->m_value[varSize] = 0;
    setValue(value);
}

VARCHAR::VARCHAR( VARCHAR &var )
{
    setVarType(TYPE_VARCHAR);
    m_wasTruncated = false;
    this->m_varSize = var.getVarSize();
    this->m_value = (char *)malloc(m_varSize+1);
    this->m_value[m_varSize] = 0;
    setValue(var.getValue());
}

VARCHAR::~VARCHAR()
{
    free(this->m_value);
}

std::string VARCHAR::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_value;
}

bool VARCHAR::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    bool r = true;
    size_t szVar = value.size();

    m_wasTruncated = false;

    if (szVar>m_varSize)
    {
        szVar = m_varSize;
        r = false;
        m_wasTruncated = true;
    }

    if (szVar>0)
    {
        this->m_value[szVar]=0;
        memcpy( this->m_value, value.c_str(), szVar );
    }
    else
        this->m_value[0]=0;


    return r;
}

char *VARCHAR::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_value;
}

bool VARCHAR::setValue(char *value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    bool r = true;

    size_t szVar = strnlen(value,m_varSize+1);
    m_wasTruncated = false;

    if (szVar>m_varSize)
    {
        szVar = m_varSize;
        r = false;
        m_wasTruncated = true;
    }

    if (szVar>0)
    {
        this->m_value[szVar]=0;
        memcpy( this->m_value, value, szVar );
    }
    else
        this->m_value[0]=0;
    return r;
}

size_t VARCHAR::getVarSize()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_varSize;
}

bool VARCHAR::getWasTruncated()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_wasTruncated;
}

unsigned long VARCHAR::getFillSize() const
{
    return m_fillSize;
}

std::shared_ptr<Var> VARCHAR::protectedCopy()
{
    Threads::Sync::Lock_RD lock(this->m_mutex);
    auto var = std::make_shared<VARCHAR>(this->m_varSize, this->m_value);
    return var;
}
