#include "identitymanager_db.h"

#include <limits>
#include <Mantids29/Threads/lock_shared.h>

#include <Mantids29/Memory/a_string.h>
#include <Mantids29/Memory/a_datetime.h>
#include <Mantids29/Memory/a_bool.h>
#include <Mantids29/Memory/a_int32.h>
#include <Mantids29/Memory/a_uint32.h>
#include <Mantids29/Memory/a_uint64.h>
#include <Mantids29/Memory/a_var.h>

using namespace Mantids29::Auth;
using namespace Mantids29::Memory;
using namespace Mantids29::Database;


Credential IdentityManager_DB::AuthController_DB::retrieveCredential(const std::string &accountName, uint32_t slotId, bool *accountFound, bool *authSlotFound)
{
    Credential ret;
    *authSlotFound = false;
    *accountFound = false;

    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::UINT32 badAttempts;
    Abstract::BOOL forcedExpiration;
    Abstract::DATETIME expiration;
    Abstract::STRING salt,hash;

    *accountFound = _parent->users->doesAccountExist(accountName);

    if (!*accountFound)
        return ret;

    auto authSlots = listAuthenticationSlots();

    if (authSlots.find(slotId) == authSlots.end())
    {
        ret.slotDetails = authSlots[slotId];
    }
    else
    {
        // Bad...
        return ret;
    }

    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `forcedExpiration`,`expiration`,`badAttempts`,`salt`,`hash` FROM vauth_v4_accountCredentials "
                                                                                      "WHERE `f_userName`=:userName AND `f_AuthSlotId`=:slotId LIMIT 1;",
                                                                                      { {":userName",new Memory::Abstract::STRING(accountName)},
                                                                                          {":slotId",new Memory::Abstract::UINT32(slotId)}
                                                                                      },
                                                                                      {  &forcedExpiration, &expiration, &badAttempts, &salt, &hash });

    if (i->getResultsOK() && i->query->step())
    {
        *authSlotFound = true;
        ret.forceExpiration = forcedExpiration.getValue();
        ret.expirationTimestamp = expiration.getValue();
        ret.badAttempts = badAttempts.getValue();
        Mantids29::Helpers::Encoders::fromHex(salt.getValue(),ret.ssalt,4);
        ret.hash = hash.getValue();
    }
    return ret;
}

bool IdentityManager_DB::AuthController_DB::validateApplicationPermissionOnRole(const std::string &roleName, const ApplicationPermission &permission, bool lock)
{
    bool ret = false;
    if (lock) _parent->m_mutex.lockShared();

    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_roleName` FROM vauth_v4_applicationPermissionsAtRole WHERE `f_permissionId`=:permissionId AND `f_appName`=:appName AND `f_roleName`=:roleName;",
                                                                                      {{":permissionId", new Memory::Abstract::STRING(permission.permissionId)}, {":appName", new Memory::Abstract::STRING(permission.appName)}, {":roleName", new Memory::Abstract::STRING(roleName)}},
                                                                                      {});
    ret = (i->getResultsOK()) && i->query->step();

    if (lock) _parent->m_mutex.unlockShared();
    return ret;
}

std::set<ApplicationPermission> IdentityManager_DB::AuthController_DB::getRoleApplicationPermissions(const std::string &roleName, bool lock)
{
    std::set<ApplicationPermission> ret;
    if (lock) _parent->m_mutex.lockShared();

    Abstract::STRING sAppName,sPermissionName;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_appName`,`f_permissionId` FROM vauth_v4_applicationPermissionsAtRole WHERE `f_roleName`=:roleName;",
                                                                                      { {":roleName",new Memory::Abstract::STRING(roleName)} },
                                                                                      { &sAppName,&sPermissionName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert({sAppName.getValue(),sPermissionName.getValue()});
    }

    if (lock) _parent->m_mutex.unlockShared();
    return ret;
}

bool IdentityManager_DB::AuthController_DB::changeCredential(const std::string &accountName, Credential passwordData, uint32_t slotId)
{
    auto authSlots = listAuthenticationSlots();

    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    if (authSlots.find(slotId) != authSlots.end())
    {
        // Bad, no slot id...
        return false;
    }

    if (! authSlots[slotId].isCompatible(passwordData.slotDetails))
    {
        // Bad slotId function...
        return false;
    }

    if (passwordData.expirationTimestamp == 1)
    {
        passwordData.expirationTimestamp = time(nullptr) + authSlots[slotId].defaultExpirationSeconds;
    }




    // TODO: evitar un cambio de password si no estas autenticado en una sesión...
    // TODO: strenght validator.

    // Destroy (if exist).
    _parent->m_sqlConnector->query("DELETE FROM vauth_v4_accountCredentials WHERE `f_userName`=:userName and `f_AuthSlotId`=:slotId",
                                   {
                                       {":userName",new Abstract::STRING(accountName)},
                                       {":slotId",new Abstract::UINT32(slotId)}
                                   });

    return _parent->m_sqlConnector->query("INSERT INTO vauth_v4_accountCredentials "
                                          "(`f_AuthSlotId`,`f_userName`,`hash`,`expiration`,`salt`,`forcedExpiration`) "
                                          "VALUES"
                                          "(:slotId,:account,:hash,:expiration,:salt,:forcedExpiration);",
                                          {
                                              {":slotId",new Abstract::UINT32(slotId)},
                                              {":account",new Abstract::STRING(accountName)},
                                              {":hash",new Abstract::STRING(passwordData.hash)},
                                              {":expiration",new Abstract::DATETIME(passwordData.expirationTimestamp)},
                                              {":salt",new Abstract::STRING(Mantids29::Helpers::Encoders::toHex(passwordData.ssalt,4))},
                                              {":forcedExpiration",new Abstract::BOOL(passwordData.forceExpiration)}
                                          });

    //                                              {":passwordFunction",new Abstract::UINT32(passwordData.passwordFunction)},
    //                                               {":totp2FAStepsToleranceWindow",new Abstract::UINT32(passwordData.totp2FAStepsToleranceWindow)}


}

std::string IdentityManager_DB::AuthController_DB::getAccountConfirmationToken(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING token;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT confirmationToken FROM vauth_v4_accountsActivationToken WHERE `f_userName`=:userName LIMIT 1;",
                                                                                      { {":userName",new Memory::Abstract::STRING(accountName)} },
                                                                                      { &token });
    if (i->getResultsOK() && i->query->step())
    {
        return token.getValue();
    }
    return "";
}


void IdentityManager_DB::AuthController_DB::updateAccountLastLogin(const std::string &accountName, const uint32_t &slotId, const ClientDetails &clientDetails)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    // Intenta actualizar primero
    bool updated = _parent->m_sqlConnector->query(
        "UPDATE vauth_v4_accountsLastLog SET `lastLogin`=CURRENT_TIMESTAMP WHERE `f_username`=:userName;",
        { {":userName", new Abstract::STRING(accountName)} }
        );

    // Si no se actualizó ningún registro, entonces inserta uno nuevo
    if (!updated)
    {
        _parent->m_sqlConnector->query(
            "INSERT INTO vauth_v4_accountsLastLog(`f_username`, `lastLogin`) VALUES (:userName, CURRENT_TIMESTAMP);",
            { {":userName", new Abstract::STRING(accountName)} }
            );
    }

    // Insertar en el registro de inicios de sesión
    _parent->m_sqlConnector->query(
        "INSERT INTO vauth_v4_accountAuthLog(`f_userName`, `f_AuthSlotId`, `loginDateTime`, `loginIP`, `loginTLSCN`, `loginUserAgent`, `loginExtraData`) "
        "VALUES (:userName, :slotId, :date, :loginIP, :loginTLSCN, :loginUserAgent, :loginExtraData);",
        {
            {":userName", new Abstract::STRING(accountName)},
            {":slotId", new Abstract::UINT32(slotId)},
            {":date", new Abstract::DATETIME(time(nullptr))},
            {":loginIP", new Abstract::STRING(clientDetails.ipAddress)},
            {":loginTLSCN", new Abstract::STRING(clientDetails.tlsCommonName)},
            {":loginUserAgent", new Abstract::STRING(clientDetails.userAgent)},
            {":loginExtraData", new Abstract::STRING(clientDetails.extraData)}
        }
        );
}

time_t IdentityManager_DB::AuthController_DB::getAccountLastLogin(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::DATETIME lastLogin;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect(
        "SELECT `lastLogin` FROM vauth_v4_accountsLastLog WHERE `f_username`=:userName LIMIT 1;",
        { {":userName", new Memory::Abstract::STRING(accountName)} },
        { &lastLogin }
        );

    if (i->getResultsOK() && i->query->step())
    {
        return lastLogin.getValue(); // Asegúrate de convertir a `time_t` si es necesario
    }

    // Si no hay registro de último inicio de sesión, retorna el valor máximo de time_t
    return std::numeric_limits<time_t>::max();
}

std::set<uint32_t> IdentityManager_DB::AuthController_DB::listUsedAuthenticationSlotsOnAccount(const std::string &accountName)
{
    std::set<uint32_t> r;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::UINT32 slotId;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_AuthSlotId` FROM vauth_v4_accountCredentials WHERE `f_userName`=:f_userName;",
                                                                                      {  {":f_userName",             new Memory::Abstract::STRING(accountName)} },
                                                                                      { &slotId });

    while (i->getResultsOK() && i->query->step())
    {
        r.insert(slotId.getValue());
    }

    return r;
}

uint32_t IdentityManager_DB::AuthController_DB::addNewAuthenticationSlot(const AuthenticationSlotDetails &details)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    auto i = _parent->m_sqlConnector->qInsert("INSERT INTO vauth_v4_authenticationSlots (`description`,`function`,`defaultExpirationSeconds`,`strenghtJSONValidator`) VALUES(:description,:function,:defaultExpirationSeconds,:strenghtJSONValidator);",
                                          {
                                              {":description",new Abstract::STRING(details.description)},
                                              {":function",new Abstract::UINT32(details.passwordFunction)},
                                              {":defaultExpirationSeconds",new Abstract::UINT32(details.defaultExpirationSeconds)},
                                              {":totp2FAStepsToleranceWindow",new Abstract::UINT32(details.totp2FAStepsToleranceWindow)},
                                              {":strenghtJSONValidator",new Abstract::STRING(details.strenghtJSONValidator)}
                                          });
    if (!i->getResultsOK())
        return std::numeric_limits<uint32_t>::max();

    return i->query->getLastInsertRowID();
}

bool IdentityManager_DB::AuthController_DB::removeAuthenticationSlot(const uint32_t &slotId)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("DELETE FROM vauth_v4_authenticationSlots WHERE `slotId`=:slotId;",
                                          {
                                              {":slotId", new Memory::Abstract::UINT32(slotId)}
                                          });
}

bool IdentityManager_DB::AuthController_DB::updateAuthenticationSlotDetails(const uint32_t &slotId, const AuthenticationSlotDetails &details)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    // Update...
    return _parent->m_sqlConnector->query(
        "UPDATE vauth_v4_authenticationSlots SET "
        "`description` = :description, "
        "`function` = :function, "
        "`defaultExpirationSeconds` = :defaultExpirationSeconds, "
        "`totp2FAStepsToleranceWindow` = :totp2FAStepsToleranceWindow, "
        "`strenghtJSONValidator` = :strenghtJSONValidator "
        "WHERE `slotId` = :slotId;",
        {
            {":slotId", new Abstract::UINT32(slotId)},
            {":description", new Abstract::STRING(details.description)},
            {":function", new Abstract::UINT32(details.passwordFunction)},
            {":defaultExpirationSeconds", new Abstract::UINT32(details.defaultExpirationSeconds)},
            {":totp2FAStepsToleranceWindow", new Abstract::UINT32(details.totp2FAStepsToleranceWindow)},
            {":strenghtJSONValidator", new Abstract::STRING(details.strenghtJSONValidator)}
        }
        );
}

std::map<uint32_t, AuthenticationSlotDetails> IdentityManager_DB::AuthController_DB::listAuthenticationSlots()
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    std::map<uint32_t, AuthenticationSlotDetails> ret;

    // Temporal Variables to store the results
    Abstract::UINT32 uSlotId;
    Abstract::STRING sDescription;
    Abstract::UINT32 uFunction;
    Abstract::UINT32 uDefaultExpirationSeconds;
    Abstract::STRING sStrengthJSONValidator;
    Abstract::UINT32 uTotp2FAStepsToleranceWindow;

    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `slotId`, `description`, `function`, `defaultExpirationSeconds`, `strengthJSONValidator`,`totp2FAStepsToleranceWindow` "
                                                                                      "FROM vauth_v4_authenticationSlots;",
                                                                                      { },
                                                                                      { &uSlotId,&sDescription,&uFunction,&uDefaultExpirationSeconds,&sStrengthJSONValidator,&uTotp2FAStepsToleranceWindow});

    // Iterate:
    while (i->getResultsOK() && i->query->step())
    {
        // Build AuthenticationSlotDetails and insert it to the maps
        ret.insert({    uSlotId.getValue(),
                        AuthenticationSlotDetails(sDescription.getValue(),
                                              (Function)uFunction.getValue(),
                                              sStrengthJSONValidator.getValue(),
                                              uDefaultExpirationSeconds.getValue(),
                                              uTotp2FAStepsToleranceWindow.getValue())
                    });
    }

    return ret;
}

uint32_t IdentityManager_DB::AuthController_DB::addAuthenticationScheme(const std::string &description)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    auto i = _parent->m_sqlConnector->qInsert("INSERT INTO vauth_v4_authenticationSchemes (`description`) VALUES(:description);",
                                              {
                                                  {":description",new Abstract::STRING(description)}
                                              });
    if (!i->getResultsOK())
        return std::numeric_limits<uint32_t>::max();

    return i->query->getLastInsertRowID();
}

bool IdentityManager_DB::AuthController_DB::updateAuthenticationScheme(const uint32_t &schemeId, const std::string &description)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    // Update...
    return _parent->m_sqlConnector->query(
        "UPDATE vauth_v4_authenticationSchemes SET "
        "`description` = :description "
        "WHERE `schemeId` = :schemeId;",
        {
            {":schemeId", new Abstract::UINT32(schemeId)},
            {":description", new Abstract::STRING(description)}
        }
        );
}

bool IdentityManager_DB::AuthController_DB::removeAuthenticationScheme(const uint32_t &schemeId)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("DELETE FROM vauth_v4_authenticationSchemes WHERE `schemeId`=:schemeId;",
                                          {
                                              {":schemeId", new Memory::Abstract::UINT32(schemeId)}
                                          });
}

std::map<uint32_t, std::string> IdentityManager_DB::AuthController_DB::listAuthenticationSchemes()
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    std::map<uint32_t, std::string> ret;

    // Temporal Variables to store the results
    Abstract::UINT32 uSlotId;
    Abstract::STRING sDescription;

    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `schemeId`, `description` FROM vauth_v4_authenticationSchemes;",
                                                                                      { },
                                                                                      { &uSlotId,&sDescription});

    // Iterate:
    while (i->getResultsOK() && i->query->step())
    {

        ret.insert({uSlotId.getValue(), sDescription.getValue()});
    }

    return ret;
}

std::set<uint32_t> IdentityManager_DB::AuthController_DB::listAuthenticationSchemesForApplicationActivity(const std::string &appName, const std::string &activityName, bool useDefault)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    std::set<uint32_t> ret;

    // Temporal Variables to store the results
    Abstract::UINT32 uSchemeId;
    std::string sqlCondition = useDefault ? " AND `isDefault`='TRUE'" : "";

    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_schemeId` FROM vauth_v4_applicationActiviesAuthSchemes WHERE `f_appName`=:appName AND `f_activityName`=:activityName" + sqlCondition + ";",
                                                                                      {
                                                                                       {":appName", new Memory::Abstract::STRING(appName)},
                                                                                       {":activityName", new Memory::Abstract::STRING(activityName)}
                                                                                      },
                                                                                      { &uSchemeId});

    // Iterate:
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(uSchemeId.getValue());
    }

    return ret;
}

bool IdentityManager_DB::AuthController_DB::addAuthenticationSchemesToApplicationActivity(const std::string &appName, const std::string &activityName, uint32_t schemeId, bool isDefault)
{
    // Acquire a write lock since we are modifying the database
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    // Execute the query using direct parameter passing...
    return _parent->m_sqlConnector->query("INSERT INTO vauth_v4_applicationActiviesAuthSchemes (`f_appName`, `f_activityName`, `f_schemeId`,`isDefault`) "
                                          "VALUES (:appName, :activityName, :schemeId, :isDefault);",
                                          {
                                                          {":appName", new Memory::Abstract::STRING(appName)},
                                                          {":activityName", new Memory::Abstract::STRING(activityName)},
                                                          {":schemeId", new Memory::Abstract::UINT32(schemeId)},
                                                          {":isDefault", new Memory::Abstract::BOOL(isDefault)}
                                          });
}

bool IdentityManager_DB::AuthController_DB::removeAuthenticationSchemeFromApplicationActivity(const std::string &appName, const std::string &activityName, uint32_t schemeId)
{
    // Acquire a write lock since we are modifying the database
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    // Execute the query with direct parameter passing
    return _parent->m_sqlConnector->query("DELETE FROM vauth_v4_applicationActiviesAuthSchemes "
                                                 "WHERE `f_appName` = :appName AND `f_activityName` = :activityName AND `f_schemeId` = :schemeId;"
                                                 , {
                                                          {":appName", new Memory::Abstract::STRING(appName)},
                                                          {":activityName", new Memory::Abstract::STRING(activityName)},
                                                          {":schemeId", new Memory::Abstract::UINT32(schemeId)}
                                          });
}

std::list<AuthenticationSchemeUsedSlot> IdentityManager_DB::AuthController_DB::listAuthenticationSlotsUsedByScheme(const uint32_t &schemeId)
{
    std::list<AuthenticationSchemeUsedSlot> slotsList;
    auto allAuthSlots = listAuthenticationSlots();

    // Acquire a read lock for thread-safe read operation
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::UINT32 uSlotId;
    Abstract::UINT32 uOrderPriority;
    Abstract::BOOL uOptional;

    // Prepare the SQL SELECT statement to fetch slot details by schemeId
    std::string sql = "SELECT `f_slotId`, `orderPriority`, `optional` "
                      "FROM `vauth_v4_authenticationSchemeUsedSlots` "
                      "WHERE `f_schemeId` = :schemeId "
                      "ORDER BY `orderPriority` ASC;"; // Assuming you want to order by priority

    // Execute the query with direct parameter passing
    std::shared_ptr<SQLConnector::QueryInstance> queryInstance = _parent->m_sqlConnector->qSelect(sql, {
                                                                                                           {":schemeId", new Memory::Abstract::UINT32(schemeId)}
                                                                                                       },
                                                                                                       {
                                                                                                            &uSlotId, &uOrderPriority, &uOptional
                                                                                                       }
                                                                                                    );

    // Assuming queryInstance->query provides a way to iterate over results and bind columns to variables

    while (queryInstance->getResultsOK() && queryInstance->query->step())
    {
        uint32_t slotId = uSlotId.getValue(), orderPriority = uOrderPriority.getValue();
        bool optional = uOptional.getValue();

        // Add the fetched slot details to the list
        if (allAuthSlots.find(slotId) != allAuthSlots.end())
        slotsList.push_back(AuthenticationSchemeUsedSlot{slotId, orderPriority, optional, allAuthSlots[slotId] });
    }

    return slotsList;
}

bool IdentityManager_DB::AuthController_DB::updateAuthenticationSlotUsedByScheme(const uint32_t &schemeId, const std::list<AuthenticationSchemeUsedSlot> &slotsUsedByScheme)
{
    // Acquire a write lock for thread-safe database modification
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    // Remove existing slots for the scheme
    std::string deleteSql = "DELETE FROM `vauth_v4_authenticationSchemeUsedSlots` WHERE `f_schemeId` = :schemeId;";
    if (!_parent->m_sqlConnector->query(deleteSql, { {":schemeId", new Memory::Abstract::UINT32(schemeId)} })) {
        return false; // If deletion fails, return false
    }

    // Repopulate the table with new slots
    for (const auto& slot : slotsUsedByScheme) {
        std::string insertSql = "INSERT INTO `vauth_v4_authenticationSchemeUsedSlots` (`f_schemeId`, `f_slotId`, `orderPriority`, `optional`) VALUES (:schemeId, :slotId, :orderPriority, :optional);";
        if (!_parent->m_sqlConnector->query(insertSql, {
                                                           {":schemeId", new Memory::Abstract::UINT32(schemeId)},
                                                           {":slotId", new Memory::Abstract::UINT32(slot.slotId)},
                                                           {":orderPriority", new Memory::Abstract::UINT32(slot.orderPriority)},
                                                           {":optional", new Memory::Abstract::BOOL(slot.optional)}
                                                       })) {
            // If any insert fails, there's limited context here to handle rollback or partial success scenarios
            return false;
        }
    }

    return true; // Return true if deletion and all insert operations succeed
}

/*
std::set<uint32_t> IdentityManager_DB::AuthController_DB::listAuthenticationSlotsUsedByScheme(const std::string &schemeId)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    std::set<uint32_t> ret;

    // Temporal Variables to store the results
    Abstract::UINT32 uSchemeId;

    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_schemeId` FROM vauth_v4_applicationActiviesAuthSchemes WHERE `f_appName`=:appName AND `f_activityName`=:activityName;",
                                                                                      {
                                                                                          {":appName", new Memory::Abstract::STRING(appName)},
                                                                                          {":activityName", new Memory::Abstract::STRING(activityName)}
                                                                                      },
                                                                                      { &uSchemeId});

    // Iterate:
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(uSchemeId.getValue());
    }

    return ret;
}
*/

void IdentityManager_DB::AuthController_DB::resetBadAttemptsOnCredential(const std::string &accountName, const uint32_t &slotId)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    _parent->m_sqlConnector->query("UPDATE vauth_v4_accountCredentials SET `badAttempts`='0' WHERE `f_userName`=:userName and `f_AuthSlotId`=:slotId;",
                                   {
                                       {":userName",new Abstract::STRING(accountName)},
                                       {":slotId",new Abstract::UINT32(slotId)}
                                   });
}

void IdentityManager_DB::AuthController_DB::incrementBadAttemptsOnCredential(const std::string &accountName, const uint32_t &slotId)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    _parent->m_sqlConnector->query("UPDATE vauth_v4_accountCredentials SET `badAttempts`=`badAttempts`+1  WHERE `f_userName`=:userName and `f_AuthSlotId`=:slotId;",
                                   {
                                       {":userName",new Abstract::STRING(accountName)},
                                       {":slotId",new Abstract::UINT32(slotId)}
                                   });
}

std::set<ApplicationPermission> IdentityManager_DB::AuthController_DB::getAccountDirectApplicationPermissions(const std::string &accountName, bool lock)
{
    std::set<ApplicationPermission> ret;
    if (lock) _parent->m_mutex.lockShared();

    Abstract::STRING appName,permission;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_appName`,`f_permissionId` FROM vauth_v4_applicationPermissionsAtAccount WHERE `f_userName`=:userName;",
                                                                                      { {":userName",new Memory::Abstract::STRING(accountName)} },
                                                                                      { &appName,&permission });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert( { appName.getValue(), permission.getValue() } );
    }

    if (lock) _parent->m_mutex.unlockShared();
    return ret;
}
