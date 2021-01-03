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
    if (!sqlConnector->dbTableExist("vauth_v2_attribs_accounts"))
    {
        bool r =
        sqlConnector->query("CREATE TABLE `vauth_v2_attribs` (\n"
                            "       `name`  VARCHAR(256) NOT NULL,\n"
                            "       `description`   VARCHAR(4096),\n"
                            "       PRIMARY KEY(`name`)\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v2_accounts` (\n"
                            "       `name`  VARCHAR(256) NOT NULL,\n"
                            "       `email` VARCHAR(1024),\n"
                            "       `description`   VARCHAR(4096),\n"
                            "       `extraData`     VARCHAR(4096),\n"
                            "       `superuser`     BOOLEAN,\n"
                            "       PRIMARY KEY(`name`)\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v2_account_activation` (\n"
                            "       `account`       VARCHAR(256) NOT NULL,\n"
                            "       `enabled`       BOOLEAN,\n"
                            "       `expiration`    DATETIME,\n"
                            "       `confirmed`     BOOLEAN,\n"
                            "       `confirmationToken`       VARCHAR(256) NOT NULL,\n"
                            "       PRIMARY KEY(`account`),\n"
                            "       FOREIGN KEY(`account`) REFERENCES vauth_v2_accounts(`name`) ON DELETE CASCADE\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v2_account_passwords` (\n"
                            "       `index` INTEGER NOT NULL,\n"
                            "       `account`       VARCHAR(256) NOT NULL,\n"
                            "       `hash`  VARCHAR(256),\n"
                            "       `expiration`    DATETIME DEFAULT NULL,\n"
                            "       `function`  INTEGER DEFAULT 0,\n"
                            "       `salt`  VARCHAR(256),\n"
                            "       `forcedExpiration`      BOOLEAN DEFAULT 0,\n"
                            "       `steps` INTEGER DEFAULT 0,\n"
                            "       PRIMARY KEY(`index`,`account`),\n"
                            "       FOREIGN KEY(`account`) REFERENCES vauth_v2_accounts(`name`) ON DELETE CASCADE\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE \"vauth_v2_groups\" (\n"
                            "       `name`  VARCHAR(256) NOT NULL,\n"
                            "       `description`   VARCHAR(4096),\n"
                            "       PRIMARY KEY(`name`)\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v2_groups_accounts` (\n"
                            "       `group_name`    VARCHAR(256) NOT NULL,\n"
                            "       `account_name`  VARCHAR(256) NOT NULL,\n"
                            "       FOREIGN KEY(`group_name`) REFERENCES vauth_v2_groups(`name`) ON DELETE CASCADE,\n"
                            "       FOREIGN KEY(`account_name`) REFERENCES vauth_v2_accounts(`name`) ON DELETE CASCADE\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v2_attribs_groups` (\n"
                            "       `attrib_name`   VARCHAR(256) NOT NULL,\n"
                            "       `group_name`    VARCHAR(256) NOT NULL,\n"
                            "       FOREIGN KEY(`attrib_name`) REFERENCES vauth_v2_attribs(`name`) ON DELETE CASCADE,\n"
                            "       FOREIGN KEY(`group_name`) REFERENCES vauth_v2_groups(`name`) ON DELETE CASCADE\n"
                            ");\n") &&
        sqlConnector->query("CREATE TABLE `vauth_v2_attribs_accounts` (\n"
                            "       `attrib_name`    VARCHAR(256) NOT NULL,\n"
                            "       `account_name`   VARCHAR(256) NOT NULL,\n"
                            "       FOREIGN KEY(`attrib_name`) REFERENCES vauth_v2_attribs(`name`) ON DELETE CASCADE,\n"
                            "       FOREIGN KEY(`account_name`) REFERENCES vauth_v2_accounts(`name`) ON DELETE CASCADE\n"
                            ");\n") &&
        // TODO: check if this needs different implementation across different databases
        sqlConnector->query("CREATE UNIQUE INDEX `idx_groups_accounts` ON `vauth_v2_groups_accounts` (`group_name` ,`account_name`);\n") &&
        sqlConnector->query("CREATE UNIQUE INDEX `idx_attribs_groups` ON `vauth_v2_attribs_groups` (`attrib_name`,`group_name` );\n") &&
        sqlConnector->query("CREATE UNIQUE INDEX `idx_attribs_accounts` ON `vauth_v2_attribs_accounts` (`attrib_name`,`account_name`);\n");
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
