#ifndef HIVE7_PERMISSIONS_H
#define HIVE7_PERMISSIONS_H

#include <string>
#include <map>

namespace CX2 { namespace Files { namespace Hive7 {

struct sH7FilePermission
{
    bool sticky;
    bool setuid;
    bool special;
    bool modify;
    bool execute;
    bool write;
    bool read;
};
class Permissions
{
public:
    Permissions();

protected:
    std::string owner, group;

    sH7FilePermission permsOwner,permsGroup,permsOthers;

    std::map<std::string, sH7FilePermission> permsACLUsers,permsACLGroups;
};

}}}

#endif // HIVE7_PERMISSIONS_H
