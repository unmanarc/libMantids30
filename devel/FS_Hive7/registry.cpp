#include "registry.h"

using namespace Mantids::Files::Hive7;


Registry::Registry()
{
    type = H7_REG_EMPTY;
}

eH7RegistryType Registry::getType() const
{
    return type;
}
