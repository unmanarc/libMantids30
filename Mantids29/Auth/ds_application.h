#ifndef DS_APPLICATION_H
#define DS_APPLICATION_H

#include <string>
#include <stdint.h>

namespace Mantids29 {
namespace Auth {

struct ApplicationDetails
{
    ApplicationDetails() {}
    std::string applicationName;
    std::string appCreator;
    std::string description;
};
struct ApplicationTokenProperties
{
    std::string appName;
    uint64_t accessTokenTimeout;
    uint64_t tempMFATokenTimeout;
    uint64_t sessionInactivityTimeout;
    std::string tokenType;
    //std::string accessTokenSigningKey;
    bool includeApplicationPermissionsInToken;
    bool includeBasicUserInfoInToken;
    bool maintainRevocationAndLogoutInfo;
};
struct ApplicationPermissionDetails
{
    ApplicationPermissionDetails() {}
    std::string permissionId;
    std::string description;
};
}}


#endif // DS_APPLICATION_H
