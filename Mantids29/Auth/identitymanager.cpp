#include "identitymanager.h"

using namespace Mantids29::Auth;


IdentityManager::IdentityManager()
{
}

IdentityManager::~IdentityManager()
{
    if (users)
        delete users;
    if (roles)
        delete roles;
    if (applications)
        delete applications;
    if (authController)
        delete authController;
}

bool IdentityManager::Users::createAdminAccount()
{
    Mantids29::Auth::AccountFlags accountFlags;
    accountFlags.confirmed = true;
    accountFlags.enabled = true;
    accountFlags.superuser = true;
    accountFlags.blocked = false;

    if (!addAccount("admin",  0, accountFlags))
    {
        return false;
    }
    return true;
}
