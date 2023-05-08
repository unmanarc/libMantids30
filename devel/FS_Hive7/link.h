#pragma once

#include "permissions.h"
#include "attributes.h"

namespace Mantids { namespace Files { namespace Hive7 {

class Link : public Attributes, public Permissions
{
public:
    Link();
};

}}}

