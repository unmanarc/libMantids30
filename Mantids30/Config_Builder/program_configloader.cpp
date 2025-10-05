#include "program_configloader.h"
#include <Mantids30/Helpers/file.h>

#include <optional>
#include <boost/property_tree/info_parser.hpp>


std::optional<boost::property_tree::ptree> Mantids30::Program::Config::Loader::loadSecureApplicationConfig(Mantids30::Program::Logs::AppLog * log, const std::string &dir, const std::string &filePath)
{
    boost::property_tree::ptree pConfig;

    std::string configFile = dir + "/" + filePath;

    log->log0(__func__, Program::Logs::LEVEL_INFO, "Loading configuration: %s", (configFile).c_str());

    if (access(dir.c_str(), R_OK))
    {
        log->log0(__func__, Program::Logs::LEVEL_CRITICAL, "Missing configuration dir: %s", dir.c_str());
        return std::nullopt;
    }

    //chdir(dir.c_str());
    bool isConfigFileInsecure = true;
    if (!Helpers::File::isSensitiveConfigPermissionInsecure(configFile, &isConfigFileInsecure))
    {
        log->log0(__func__, Program::Logs::LEVEL_WARN, "The configuration '%s' file is inaccessible, loading defaults...", configFile.c_str());
    }
    else
    {
        if (isConfigFileInsecure)
        {
            log->log0(__func__, Program::Logs::LEVEL_SECURITY_ALERT,
                          "The permissions of the '%s' file are currently not set to 0600. This may leave your API key exposed to potential security threats. To mitigate this risk, "
                          "we are changing the permissions of the file to ensure that your API key remains secure. Please ensure that you take necessary precautions to protect your API key and "
                          "update any affected applications or services as necessary.",configFile.c_str());

            if (Helpers::File::fixSensitiveConfigPermission(configFile))
            {
                log->log0(__func__, Program::Logs::LEVEL_SECURITY_ALERT, "The permissions of the '%s' file has been changed to 0600.",configFile.c_str());
            }
            else
            {
                log->log0(__func__, Program::Logs::LEVEL_CRITICAL, "The permissions of the '%s' file can't be changed.",configFile.c_str());
                return std::nullopt;
            }
        }

        boost::property_tree::info_parser::read_info(configFile, pConfig);
    }

    return pConfig;
}
