#include "identitymanager.h"

#include <Mantids29/Threads/lock_shared.h>
#include <Mantids29/Helpers/random.h>

using namespace Mantids29::Auth;

Reason IdentityManager::AuthController::authenticateCredential(const ClientDetails &clientDetails, const std::string &accountName, const std::string &incommingPassword, uint32_t slotId, Mode authMode, const std::string &challengeSalt)
{
    Reason ret = REASON_BAD_ACCOUNT;
    bool accountFound=false, authSlotFound=false;
    Credential pStoredCredentialData;
    uint32_t currentAuthPolicy_MaxTries=0;

    // If something changes in between,
    if (1)
    {
        Threads::Sync::Lock_RD lock(m_parent->m_mutex);

/*        // TODO: esto no debe ir aquí... ya que es un SSO, el usuario se puede autenticar de forma independiente a la app...
        // Check if the user is enabled to authenticate in this APP:
        if (!m_parent->applications->validateApplicationAccount(appName,accountName))
            return REASON_BAD_PASSWORD; // Account not available for this application. (prevent accounts enumeration)*/

        // Check if the retrieved credential
        pStoredCredentialData = retrieveCredential(accountName,slotId, &accountFound, &authSlotFound);
        currentAuthPolicy_MaxTries = m_authenticationPolicy.maxTries;

        if (accountFound == false)
            ret = REASON_BAD_ACCOUNT;
        else if (authSlotFound == false)
            ret = REASON_PASSWORD_INDEX_NOTFOUND;
        else
        {
            time_t lastLogin = getAccountLastLogin(accountName);
            auto flags = m_parent->users->getAccountFlags(accountName);

            if      (!flags.confirmed)
                return REASON_UNCONFIRMED_ACCOUNT;

            else if (!flags.enabled)
                return REASON_DISABLED_ACCOUNT;

            else if (flags.blocked)
                return REASON_DISABLED_ACCOUNT;

            else if (m_parent->users->isAccountExpired(accountName))
                return REASON_EXPIRED_ACCOUNT;

            else if (lastLogin+m_authenticationPolicy.abandonedAccountExpirationSeconds<time(nullptr))
                return REASON_EXPIRED_ACCOUNT;

            else
            {
                ret = validateStoredCredential(accountName,pStoredCredentialData, incommingPassword, challengeSalt, authMode);
            }
        }
    }

    // Register the change for max attempts...
    if ( !IS_PASSWORD_AUTHENTICATED( ret ) )
    {
        // Increment the counter and disable the account acording to the policy.
        if ( (pStoredCredentialData.badAttempts + 1) >= currentAuthPolicy_MaxTries )
        {
            // Disable the account...
            m_parent->users->disableAccount(accountName,true);
        }
        else
        {
            incrementBadAttemptsOnCredential(accountName,slotId);
        }
    }
    else
    {
        // Authenticated:
        updateAccountLastLogin(accountName,slotId,clientDetails);
        resetBadAttemptsOnCredential(accountName,slotId);
    }


    return ret;
}

std::string IdentityManager::AuthController::genRandomConfirmationToken()
{
    return Mantids29::Helpers::Random::createRandomString(64);
}


AuthenticationPolicy IdentityManager::AuthController::getAuthenticationPolicy()
{
    Threads::Sync::Lock_RD lock(m_parent->m_mutex);
    return m_authenticationPolicy;
}

void IdentityManager::AuthController::setAuthenticationPolicy(const AuthenticationPolicy &newAuthenticationPolicy)
{
    Threads::Sync::Lock_RW lock(m_parent->m_mutex);
    m_authenticationPolicy = newAuthenticationPolicy;
}


Credential IdentityManager::AuthController::getAccountCredentialPublicData(const std::string &accountName, uint32_t slotId)
{
    // protective-limited method.
    bool bAccountFound = false;
    bool bSlotIdFound = false;
    Credential credential = retrieveCredential(accountName, slotId, &bAccountFound, &bSlotIdFound);
    return credential.getPublicData();
}

std::map<uint32_t, Credential> IdentityManager::AuthController::getAccountAllCredentialsPublicData(const std::string &accountName)
{
    // TODO: this function can only be accessed if the user has been authenticated...
    std::map<uint32_t, Credential> r;

    std::set<uint32_t> slotIdsUsedByAccount =  listUsedAuthenticationSlotsOnAccount(accountName);
    for (const uint32_t slotId : slotIdsUsedByAccount)
    {
        bool accountFound,authSlotFound;
        Credential credential = retrieveCredential(accountName,slotId,&accountFound,&authSlotFound);
        if (accountFound && authSlotFound)
        {
            r[slotId] = credential.getPublicData();
        }
    }

    return r;
}
/*
std::map<uint32_t,AuthenticationSlotDetails> IdentityManager::AuthController::getAccountAuthenticationSlotsUsedForLogin(const std::string &accountName)
{
    std::map<uint32_t,AuthenticationSlotDetails> r;
    std::set<uint32_t> slotIdsRequiredForLogin = getAuthenticationSlotsRequiredForLogin();

    if (slotIdsRequiredForLogin.empty())
    {
        // Weird... could even be a database error... add impossible's r.
        r[0xFFFFFFFF] = AuthenticationSlotDetails();
        return r;
    }

    std::map<uint32_t,AuthenticationSlotDetails> authenticationSlots = listAuthenticationSlots();

    for (const auto & slotId : listUsedAuthenticationSlotsOnAccount(accountName))
    {
        if (slotIdsRequiredForLogin.find(slotId)!=slotIdsRequiredForLogin.end() && authenticationSlots.find(slotId)!=authenticationSlots.end())
        {
            r[slotId] = authenticationSlots[slotId];
        }
    }

    return r;
}*/


// TODO: this can only be called when authenticated.
bool IdentityManager::AuthController::changeAccountAuthenticatedCredential(const std::string &accountName, uint32_t slotId, const std::string &sCurrentPassword, const Credential &passwordData, const ClientDetails &clientInfo, Mode authMode, const std::string &challengeSalt)
{
    auto authSlots = listAuthenticationSlots();

    if ( authSlots.find(slotId) == authSlots.end() )
    {
        // Bad, no slot id available...
        return false;
    }

    if (!authSlots[slotId].isCompatible(passwordData.slotDetails))
    {
        // Bad, not compatible password...
        return false;
    }

    bool accountFound, authSlotFound;

    Credential storedCredential = retrieveCredential(accountName, slotId, &accountFound, &authSlotFound);

    if (!accountFound)
    {
        // Account not found, you can't change this password...
        return false;
    }

    if (authSlotFound)
    {
        // If the slotId has been initialized, authenticate the current credential.
        auto i = authenticateCredential(clientInfo, accountName, sCurrentPassword, slotId, authMode, challengeSalt);
        // Now take the authentication and add/change the credential
        if ( ! (IS_PASSWORD_AUTHENTICATED(i)) )
        {
            // Change the requested index.
            return false;
        }
    }

    // change it here...
    return changeCredential(accountName,passwordData,slotId);
}



bool IdentityManager::AuthController::validateAccountApplicationPermission(const std::string &accountName, const ApplicationPermission & applicationPermission)
{
    Threads::Sync::Lock_RD lock(m_parent->m_mutex);
    if (validateAccountDirectApplicationPermission(accountName,applicationPermission))
    {
        return true;
    }
    for (const std::string & roleName : m_parent->users->getAccountRoles(accountName,false))
    {
        if (validateApplicationPermissionOnRole(roleName, applicationPermission,false))
        {
            return true;
        }
    }
    return false;
}

std::set<ApplicationPermission> IdentityManager::AuthController::getAccountUsableApplicationPermissions(const std::string &accountName)
{
    std::set<ApplicationPermission> x;
    Threads::Sync::Lock_RD lock(m_parent->m_mutex);
    // Take permissions from the account
    for (const ApplicationPermission & permission : getAccountDirectApplicationPermissions(accountName,false))
        x.insert(permission);

    // Take the permissions from the belonging roles
    for (const std::string & roleName : m_parent->users->getAccountRoles(accountName,false))
    {
        for (const ApplicationPermission & permission : getRoleApplicationPermissions(roleName,false))
            x.insert(permission);
    }
    return x;
}



bool IdentityManager::AuthController::setAdminAccountPassword(std::string *sInitPW)
{
    *sInitPW = "";

    std::set<uint32_t> applicationActivitySSOLogin = m_parent->authController->listAuthenticationSchemesForApplicationActivity("LOGIN","SSO",true);

    // Not any scheme to the default
    if (applicationActivitySSOLogin.empty())
    {
        return false;
    }

    std::list<AuthenticationSchemeUsedSlot> authSlots = m_parent->authController->listAuthenticationSlotsUsedByScheme(*applicationActivitySSOLogin.begin());

    // not any slot assigned to this scheme
    if (authSlots.empty())
    {
        return false;
    }

    // not a password...
    if (!authSlots.begin()->details.isTextPasswordFunction())
    {
        return false;
    }

    std::string newPass = Mantids29::Helpers::Random::createRandomString(16);

    Auth::Credential credentialData = m_parent->authController->createNewCredential(authSlots.begin()->slotId,newPass,true);

    bool r =  m_parent->authController->changeCredential("admin",credentialData,authSlots.begin()->slotId);

    if (r)
        *sInitPW = newPass;

    return r;

    /*
            &&

            &&
           _parent->m_sqlConnector->query("INSERT INTO vauth_v4_accountCredentials "
                                "(`f_AuthSlotId`,`f_userName`,`hash`,`expiration`,`salt`,`forcedExpiration`)"
                                " VALUES"
                                "('0',:account,:hash,:expiration,:salt,:forcedExpiration);",
                                {
                                    {":account",new Abstract::STRING(accountName)},
                                    {":hash",new Abstract::STRING(credentialData.hash)},
                                    {":expiration",new Abstract::DATETIME(credentialData.expirationTimestamp)},
                                    {":salt",new Abstract::STRING(Mantids29::Helpers::Encoders::toHex(credentialData.ssalt,4))},
                                    {":forcedExpiration",new Abstract::BOOL(credentialData.forceExpiration)}
                                }
                                );*/
    // TODO: pasar esto a slot
    //`totp2FAStepsToleranceWindow`,`function`

}

Credential IdentityManager::AuthController::createNewCredential(const uint32_t & slotId, const std::string & passwordInput, bool forceExpiration)
{
    Credential r;

    auto authSlots = listAuthenticationSlots();

    if (authSlots.find(slotId) == authSlots.end())
        return r;

    r.slotDetails = authSlots[slotId];
    r.forceExpiration = forceExpiration;
    r.expirationTimestamp = time(nullptr)+r.slotDetails.defaultExpirationSeconds;

    switch (r.slotDetails.passwordFunction)
    {
    case FN_NOTFOUND:
    {
        // Do nothing...
    } break;
    case FN_PLAIN:
    {
        r.hash = passwordInput;
    } break;
    case FN_SHA256:
    {
        r.hash = Helpers::Crypto::calcSHA256(passwordInput);
    } break;
    case FN_SHA512:
    {
        r.hash = Helpers::Crypto::calcSHA512(passwordInput);
    } break;
    case FN_SSHA256:
    {
        Mantids29::Helpers::Random::createRandomSalt32(r.ssalt);
        r.hash = Helpers::Crypto::calcSSHA256(passwordInput, r.ssalt);
    } break;
    case FN_SSHA512:
    {
        Mantids29::Helpers::Random::createRandomSalt32(r.ssalt);
        r.hash = Helpers::Crypto::calcSSHA512(passwordInput, r.ssalt);
    } break;
    case FN_GAUTHTIME:
        r.hash = passwordInput;
    }

    return r;
}

