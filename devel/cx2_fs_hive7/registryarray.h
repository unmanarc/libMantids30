#ifndef HIVE7_REGARRAY_H
#define HIVE7_REGARRAY_H

#include "attributes.h"
#include "permissions.h"

namespace CX2 { namespace Files { namespace Hive7 {
class RegistryArray : public Attributes, public Permissions
{
public:
    RegistryArray();
};

}}}

#endif // HIVE7_REGARRAY_H
