#ifndef HIVE7_LINK_H
#define HIVE7_LINK_H

#include "permissions.h"
#include "attributes.h"

namespace CX2 { namespace Files { namespace Hive7 {

class Link : public Attributes, public Permissions
{
public:
    Link();
};

}}}

#endif // HIVE7_LINK_H
