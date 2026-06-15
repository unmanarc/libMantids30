#pragma once

#include "attributes.h"
#include "permissions.h"

namespace Mantids {
namespace Files {
namespace Hive7 {

class Link : public Attributes, public Permissions
{
public:
    Link();
};

} // namespace Hive7
} // namespace Files
} // namespace Mantids
