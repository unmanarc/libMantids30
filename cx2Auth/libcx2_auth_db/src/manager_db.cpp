#include "manager_db.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef WIN32
#include <pwd.h>
#endif

using namespace CX2::Authentication;

Manager_DB::Manager_DB( CX2::Database::SQLConnector * sqlConnector )
{
    this->sqlConnector = sqlConnector;
}

bool Manager_DB::initScheme()
{
    if (!sqlConnector->dbTableExist("vauth_v3_attribsaccounts"))
    {
        bool r =
        sqlConnector->query("CREATE TABLE `vauth_v3_attribs` (\n"
                            "       `attribname`  VARCHAR(256) NOT NULL,\n"
                            "       `description`   VARCHAR(4096),\n"
                            "       PRIMARY KEY(`attribname`)\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v3_accounts` (\n"
                            "       `username`  VARCHAR(256) NOT NULL,\n"
                            "       `email` VARCHAR(1024),\n"
                            "       `description`   VARCHAR(4096),\n"
                            "       `extraData`     VARCHAR(4096),\n"
                            "       `superuser`     BOOLEAN NOT NULL,\n"
                            "       `expiration`    DATETIME NOT NULL,\n"
                            "       `enabled`       BOOLEAN NOT NULL,\n"
                            "       `confirmed`     BOOLEAN NOT NULL,\n"
                            "       PRIMARY KEY(`username`)\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v3_accountactivationtokens` (\n"
                            "       `f_username`            VARCHAR(256) NOT NULL,\n"
                            "       `confirmationToken`     VARCHAR(256) NOT NULL,\n"
                            "       PRIMARY KEY(`f_username`),\n"
                            "       FOREIGN KEY(`f_username`) REFERENCES vauth_v3_accounts(`username`) ON DELETE CASCADE\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v3_accountsecrets` (\n"
                            "       `index` INTEGER NOT NULL,\n"
                            "       `f_username`       VARCHAR(256) NOT NULL,\n"
                            "       `hash`  VARCHAR(256),\n"
                            "       `expiration`    DATETIME DEFAULT NULL,\n"
                            "       `function`  INTEGER DEFAULT 0,\n"
                            "       `salt`  VARCHAR(256),\n"
                            "       `forcedExpiration`      BOOLEAN DEFAULT 0,\n"
                            "       `steps` INTEGER DEFAULT 0,\n"
                            "       PRIMARY KEY(`index`,`f_username`),\n"
                            "       FOREIGN KEY(`f_username`) REFERENCES vauth_v3_accounts(`username`) ON DELETE CASCADE\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE \"vauth_v3_groups\" (\n"
                            "       `groupname`  VARCHAR(256) NOT NULL,\n"
                            "       `description`   VARCHAR(4096),\n"
                            "       PRIMARY KEY(`groupname`)\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v3_groupsaccounts` (\n"
                            "       `f_groupname`    VARCHAR(256) NOT NULL,\n"
                            "       `f_username`  VARCHAR(256) NOT NULL,\n"
                            "       FOREIGN KEY(`f_groupname`) REFERENCES vauth_v3_groups(`groupname`) ON DELETE CASCADE,\n"
                            "       FOREIGN KEY(`f_username`) REFERENCES vauth_v3_accounts(`username`) ON DELETE CASCADE\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v3_attribsgroups` (\n"
                            "       `f_attribname`   VARCHAR(256) NOT NULL,\n"
                            "       `f_groupname`    VARCHAR(256) NOT NULL,\n"
                            "       FOREIGN KEY(`f_attribname`) REFERENCES vauth_v3_attribs(`attribname`) ON DELETE CASCADE,\n"
                            "       FOREIGN KEY(`f_groupname`) REFERENCES vauth_v3_groups(`groupname`) ON DELETE CASCADE\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v3_attribsaccounts` (\n"
                            "       `f_attribname`    VARCHAR(256) NOT NULL,\n"
                            "       `f_username`   VARCHAR(256) NOT NULL,\n"
                            "       FOREIGN KEY(`f_attribname`) REFERENCES vauth_v3_attribs(`attribname`) ON DELETE CASCADE,\n"
                            "       FOREIGN KEY(`f_username`) REFERENCES vauth_v3_accounts(`username`) ON DELETE CASCADE\n"
                            ");\n") &&
        // TODO: check if this needs different implementation across different databases
        sqlConnector->query("CREATE UNIQUE INDEX `idx_groups_accounts` ON `vauth_v3_groupsaccounts` (`f_groupname` ,`f_username`);\n") &&
        sqlConnector->query("CREATE UNIQUE INDEX `idx_attribs_groups` ON `vauth_v3_attribsgroups` (`f_attribname`,`f_groupname` );\n") &&
        sqlConnector->query("CREATE UNIQUE INDEX `idx_attribs_accounts` ON `vauth_v3_attribsaccounts` (`f_attribname`,`f_username`);\n");
        return r;
    }
    return true;
}

std::list<std::string> Manager_DB::getSqlErrorList() const
{
    return sqlErrorList;
}

void Manager_DB::clearSQLErrorList()
{
    sqlErrorList.clear();
}
