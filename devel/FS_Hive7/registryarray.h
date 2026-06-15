#pragma once

#include "attributes.h"
#include "permissions.h"

namespace Mantids {
namespace Files {
namespace Hive7 {
class RegistryArray : public Attributes, public Permissions
{
public:
    RegistryArray();
};

} // namespace Hive7
} // namespace Files
} // namespace Mantids
