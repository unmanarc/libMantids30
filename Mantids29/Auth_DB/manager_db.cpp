#include "manager_db.h"

#include <Mantids29/Memory/a_uint32.h>
#include <Mantids29/Memory/a_string.h>
#include <Mantids29/Memory/a_bool.h>


#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _WIN32
#include <pwd.h>
#endif

using namespace Mantids29::Authentication;

Manager_DB::Manager_DB( Mantids29::Database::SQLConnector * sqlConnector )
{
    this->m_sqlConnector = sqlConnector;
}

bool Manager_DB::initScheme()
{
    if (!m_sqlConnector->dbTableExist("vauth_v4_attribsaccounts"))
    {
        bool r =
                m_sqlConnector->query("CREATE TABLE `vauth_v4_accounts` (\n"
                                      "       `userName`              VARCHAR(256)    NOT NULL,\n"
                                      "       `givenName`             VARCHAR(256)            ,\n"
                                      "       `lastName`              VARCHAR(256)            ,\n"
                                      "       `email`                 VARCHAR(1024)           ,\n"
                                      "       `description`           VARCHAR(4096)           ,\n"
                                      "       `extraData`             VARCHAR(4096)           ,\n"
                                      "       `isSuperuser`           BOOLEAN         NOT NULL,\n"
                                      "       `creation`              DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,\n"
                                      "       `creator`               VARCHAR(256)    DEFAULT NULL,\n"
                                      "       `expiration`            DATETIME        NOT NULL,\n"
                                      "       `lastLogin`             DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,\n"
                                      "       `isEnabled`             BOOLEAN         NOT NULL,\n"
                                      "       `isConfirmed`           BOOLEAN         NOT NULL,\n"
                                      "       PRIMARY KEY(`userName`)\n"
                                      ");\n") &&

                m_sqlConnector->query("CREATE TABLE `vauth_v4_applications` (\n"
                                      "       `appName`               VARCHAR(256)  NOT NULL,\n"
                                      "       `f_appCreator`          VARCHAR(256)  NOT NULL,\n"
                                      "       `appDescription`        VARCHAR(4096) NOT NULL,\n"
                                      "       `apiKey`                VARCHAR(512)  NOT NULL,\n"
                                      "       FOREIGN KEY(`f_appCreator`)   REFERENCES vauth_v4_accounts(`userName`) ON DELETE CASCADE\n"
                                      "       PRIMARY KEY(`appName`)\n"
                                      ");\n") &&

                m_sqlConnector->query("CREATE TABLE `vauth_v4_applications_weblogin_returls` (\n"
                                      "       `f_appName`             VARCHAR(256)  NOT NULL,\n"
                                      "       `loginReturnUrl`         VARCHAR(2048) NOT NULL,\n"
                                      "       FOREIGN KEY(`f_appName`)   REFERENCES vauth_v4_applications(`appName`) ON DELETE CASCADE\n"
                                      "       PRIMARY KEY(`f_appName`,`loginReturnURL`)\n"
                                      ");\n") &&

                m_sqlConnector->query("CREATE TABLE `vauth_v4_applications_login_origins` (\n"
                                      "       `f_appName`             VARCHAR(256)  NOT NULL,\n"
                                      "       `originUrl`              VARCHAR(2048) NOT NULL,\n"
                                      "       FOREIGN KEY(`f_appName`)   REFERENCES vauth_v4_applications(`appName`) ON DELETE CASCADE\n"
                                      "       PRIMARY KEY(`f_appName`,`originUrl`)\n"
                                      ");\n") &&

                m_sqlConnector->query("CREATE TABLE `vauth_v4_attribs` (\n"
                                      "       `f_appName`             VARCHAR(256) NOT NULL,\n"
                                      "       `attribName`            VARCHAR(256) NOT NULL,\n"
                                      "       `attribDescription`     VARCHAR(4096),\n"
                                      "       PRIMARY KEY(`f_appName`,`attribName`),\n"
                                      "       FOREIGN KEY(`f_appName`)   REFERENCES vauth_v4_applications(`appName`) ON DELETE CASCADE\n"
                                      ");\n") &&
                m_sqlConnector->query("CREATE TABLE `vauth_v4_applicationmanagers` (\n"
                                      "       `f_userNameManager`       VARCHAR(256)    NOT NULL,\n"
                                      "       `f_applicationManaged`    VARCHAR(256)    NOT NULL,\n"
                                      "       PRIMARY KEY(`f_userNameManager`,`f_applicationManaged`),\n"
                                      "       FOREIGN KEY(`f_userNameManager`)       REFERENCES vauth_v4_accounts(`userName`) ON DELETE CASCADE,\n"
                                      "       FOREIGN KEY(`f_applicationManaged`)    REFERENCES vauth_v4_applications(`appName`) ON DELETE CASCADE\n"
                                      ");\n") &&
                m_sqlConnector->query("CREATE TABLE `vauth_v4_applicationusers` (\n"
                                      "       `f_userName`       VARCHAR(256)    NOT NULL,\n"
                                      "       `f_appName`        VARCHAR(256)    NOT NULL,\n"
                                      "       PRIMARY KEY(`f_userName`,`f_appName`),\n"
                                      "       FOREIGN KEY(`f_userName`) REFERENCES vauth_v4_accounts(`userName`) ON DELETE CASCADE,\n"
                                      "       FOREIGN KEY(`f_appName`)  REFERENCES vauth_v4_applications(`appName`) ON DELETE CASCADE\n"
                                      ");\n") &&
                m_sqlConnector->query("CREATE TABLE `vauth_v4_secretsindices` (\n"
                                      "       `index`                 INTEGER       NOT NULL,\n"
                                      "       `indexDescription`      VARCHAR(4096) NOT NULL,\n"
                                      "       `isLoginRequired`         BOOLEAN       NOT NULL,\n"
                                      "       PRIMARY KEY(`index`)\n"
                                      ");\n") &&
                m_sqlConnector->query("CREATE TABLE `vauth_v4_accountlogins` (\n"
                                      "       `f_userName`        VARCHAR(256)    NOT NULL,\n"
                                      "       `f_secretIndex`     INTEGER         NOT NULL,\n"
                                      "       `loginDateTime`     DATETIME        NOT NULL,\n"
                                      "       `loginIP`           VARCHAR(64)     NOT NULL,\n"
                                      "       `loginTLSCN`        VARCHAR(1024)   NOT NULL,\n"
                                      "       `loginUserAgent`    VARCHAR(4096)   NOT NULL,\n"
                                      "       `loginExtraData`    VARCHAR(4096)   NOT NULL,\n"
                                      "       FOREIGN KEY(`f_secretIndex`)    REFERENCES vauth_v4_secretsindices(`index`) ON DELETE CASCADE\n"
                                      "       FOREIGN KEY(`f_userName`)       REFERENCES vauth_v4_accounts(`userName`) ON DELETE CASCADE\n"
                                      ");\n") &&
                m_sqlConnector->query("CREATE TABLE `vauth_v4_accountmanagers` (\n"
                                      "       `f_userNameManager`     VARCHAR(256)    NOT NULL,\n"
                                      "       `f_userName_managed`    VARCHAR(256)    NOT NULL,\n"
                                      "       PRIMARY KEY(`f_userNameManager`,`f_userName_managed`),\n"
                                      "       FOREIGN KEY(`f_userNameManager`)   REFERENCES vauth_v4_accounts(`userName`) ON DELETE CASCADE,\n"
                                      "       FOREIGN KEY(`f_userName_managed`)  REFERENCES vauth_v4_accounts(`userName`) ON DELETE CASCADE\n"
                                      ");\n") &&
                m_sqlConnector->query("CREATE TABLE `vauth_v4_accountactivationtokens` (\n"
                                      "       `f_userName`            VARCHAR(256) NOT NULL,\n"
                                      "       `confirmationToken`     VARCHAR(256) NOT NULL,\n"
                                      "       PRIMARY KEY(`f_userName`),\n"
                                      "       FOREIGN KEY(`f_userName`) REFERENCES vauth_v4_accounts(`userName`) ON DELETE CASCADE\n"
                                      ");\n") &&
                m_sqlConnector->query("CREATE TABLE `vauth_v4_accountsecrets` (\n"
                                      "       `f_secretIndex`         INTEGER         NOT NULL,\n"
                                      "       `f_userName`            VARCHAR(256)    NOT NULL,\n"
                                      "       `hash`                  VARCHAR(256)    NOT NULL,\n"
                                      "       `expiration`            DATETIME        DEFAULT NULL,\n"
                                      "       `function`              INTEGER         DEFAULT 0,\n"
                                      "       `salt`                  VARCHAR(256)            ,\n"
                                      "       `forcedExpiration`      BOOLEAN         DEFAULT 0,\n"
                                      "       `steps`                 INTEGER         DEFAULT 0,\n"
                                      "       `badAttempts`           INTEGER         DEFAULT 0,\n"
                                      "       PRIMARY KEY(`f_secretIndex`,`f_userName`),\n"
                                      "       FOREIGN KEY(`f_secretIndex`)      REFERENCES vauth_v4_secretsindices(`index`) ON DELETE CASCADE,\n"
                                      "       FOREIGN KEY(`f_userName`)         REFERENCES vauth_v4_accounts(`userName`) ON DELETE CASCADE\n"
                                      ");\n") &&
                m_sqlConnector->query("CREATE TABLE \"vauth_v4_groups\" (\n"
                                      "       `groupName`             VARCHAR(256) NOT NULL,\n"
                                      "       `groupDescription`           VARCHAR(4096),\n"
                                      "       PRIMARY KEY(`groupName`)\n"
                                      ");\n") &&
                m_sqlConnector->query("CREATE TABLE `vauth_v4_groupsaccounts` (\n"
                                      "       `f_groupName`           VARCHAR(256) NOT NULL,\n"
                                      "       `f_userName`            VARCHAR(256) NOT NULL,\n"
                                      "       FOREIGN KEY(`f_groupName`)      REFERENCES vauth_v4_groups(`groupName`) ON DELETE CASCADE,\n"
                                      "       FOREIGN KEY(`f_userName`)       REFERENCES vauth_v4_accounts(`userName`) ON DELETE CASCADE\n"
                                      ");\n") &&
                m_sqlConnector->query("CREATE TABLE `vauth_v4_attribsgroups` (\n"
                                      "       `f_appName`             VARCHAR(256) NOT NULL,\n"
                                      "       `f_attribName`          VARCHAR(256) NOT NULL,\n"
                                      "       `f_groupName`           VARCHAR(256) NOT NULL,\n"
                                      "       FOREIGN KEY(`f_appName`,`f_attribName`) REFERENCES vauth_v4_attribs(`f_appName`,`attribName`) ON DELETE CASCADE,\n"
                                      "       FOREIGN KEY(`f_groupName`)              REFERENCES vauth_v4_groups(`groupName`) ON DELETE CASCADE\n"
                                      ");\n") &&
                 m_sqlConnector->query("CREATE TABLE `vauth_v4_attribsaccounts` ("
                                       "       `f_appName`             VARCHAR(256) NOT NULL,"
                                       "       `f_attribName`          VARCHAR(256) NOT NULL,"
                                       "       `f_userName`            VARCHAR(256) NOT NULL,"
                                       "       FOREIGN KEY(`f_appName`,`f_attribName`) REFERENCES vauth_v4_attribs(`f_appName`,`attribName`) ON DELETE CASCADE,"
                                       "       FOREIGN KEY(`f_userName`, `f_appName`) REFERENCES vauth_v4_applicationusers(`f_userName`, `f_appName`) ON DELETE CASCADE"
                                       ");")
                 &&
                // TODO: check if this needs different implementation across different databases, by now this works on SQLite3
                m_sqlConnector->query("CREATE UNIQUE INDEX `idx_groups_accounts` ON `vauth_v4_groupsaccounts` (`f_groupName` ,`f_userName`);\n") &&
                m_sqlConnector->query("CREATE UNIQUE INDEX `idx_attribs_groups` ON `vauth_v4_attribsgroups` (`f_appName`,`f_attribName`,`f_groupName` );\n") &&
                m_sqlConnector->query("CREATE UNIQUE INDEX `idx_attribs_accounts` ON `vauth_v4_attribsaccounts` (`f_appName`,`f_attribName`,`f_userName`);\n") &&
                m_sqlConnector->query("INSERT INTO vauth_v4_secretsindices (`index`,`indexDescription`,`isLoginRequired`) "
                                      "VALUES(:index,:indexDescription,:loginRequired);",
                                      {
                                          {":index",             new Memory::Abstract::UINT32(0)},
                                          {":indexDescription",  new Memory::Abstract::STRING("Master Login Password")},
                                          {":loginRequired",     new Memory::Abstract::BOOL(true)}
                                      }
                                      )

                ;
        return r;
    }
    return true;
}

std::list<std::string> Manager_DB::getSqlErrorList() const
{
    return m_sqlErrorList;
}

void Manager_DB::clearSQLErrorList()
{
    m_sqlErrorList.clear();
}
