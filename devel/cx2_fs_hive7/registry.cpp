#include "registry.h"

using namespace CX2::Files::Hive7;


Registry::Registry()
{
    type = H7_REG_EMPTY;
}

eH7RegistryType Registry::getType() const
{
    return type;
}
