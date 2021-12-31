#include "a_varchar.h"
#include <string.h>
#include <mdz_thr_mutex/lock_shared.h>

using namespace Mantids::Memory::Abstract;

VARCHAR::VARCHAR(const size_t &varSize)
{
    setVarType(TYPE_VARCHAR);
    wasTruncated = false;
    this->varSize = varSize;
    this->value = (char *)malloc(varSize+1);
    this->value[varSize] = 0;
}

VARCHAR::VARCHAR(const size_t &varSize, char *value)
{
    setVarType(TYPE_VARCHAR);
    wasTruncated = false;
    this->varSize = varSize;
    this->value = (char *)malloc(varSize+1);
    this->value[varSize] = 0;
    setValue(value);
}

VARCHAR::VARCHAR( VARCHAR &var )
{
    setVarType(TYPE_VARCHAR);
    wasTruncated = false;
    this->varSize = var.getVarSize();
    this->value = (char *)malloc(varSize+1);
    this->value[varSize] = 0;
    setValue(var.getValue());
}

VARCHAR::~VARCHAR()
{
    free(this->value);
}

std::string VARCHAR::toString()
{
    Threads::Sync::Lock_RD lock(mutex);

    return value;
}

bool VARCHAR::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    bool r = true;
    size_t szVar = value.size();

    wasTruncated = false;

    if (szVar>varSize)
    {
        szVar = varSize;
        r = false;
        wasTruncated = true;
    }

    if (szVar>0)
    {
        this->value[szVar]=0;
        memcpy( this->value, value.c_str(), szVar );
    }
    else
        this->value[0]=0;


    return r;
}

char *VARCHAR::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);

    return value;
}

bool VARCHAR::setValue(char *value)
{
    Threads::Sync::Lock_RW lock(mutex);

    bool r = true;

    size_t szVar = strnlen(value,varSize+1);
    wasTruncated = false;

    if (szVar>varSize)
    {
        szVar = varSize;
        r = false;
        wasTruncated = true;
    }

    if (szVar>0)
    {
        this->value[szVar]=0;
        memcpy( this->value, value, szVar );
    }
    else
        this->value[0]=0;
    return r;
}

size_t VARCHAR::getVarSize()
{
    Threads::Sync::Lock_RD lock(mutex);

    return varSize;
}

bool VARCHAR::getWasTruncated()
{
    Threads::Sync::Lock_RD lock(mutex);

    return wasTruncated;
}

unsigned long VARCHAR::getFillSize() const
{
    return fillSize;
}

Var *VARCHAR::protectedCopy()
{
    Threads::Sync::Lock_RD lock(this->mutex);
    VARCHAR * var = new VARCHAR(this->varSize,this->value);
    return var;
}
