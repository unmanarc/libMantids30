#include "file.h"

#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#ifdef _WIN32
#include <Windows.h>
#include <Aclapi.h>
#else
#include <sys/stat.h>
#endif

std::string Mantids29::Helpers::File::loadFileIntoString(const std::string &filePath, bool *ok)
{
    if (ok)
        *ok = false;

    std::ifstream infile(filePath);
    if (!infile.is_open())
    {
        return "";
    }

    std::string fileContent =  std::string((std::istreambuf_iterator<char>(infile)),std::istreambuf_iterator<char>());

    if (ok)
        *ok = true;

    infile.close();
    return fileContent;
}

bool Mantids29::Helpers::File::writeStringToFile(const std::string &filePath, const std::string &content)
{
    std::ofstream outfile;
    outfile.open(filePath, std::ios_base::out);
    if (outfile.is_open())
    {
        outfile  << content;
        outfile.close();
        return true;
    }
    return false;
}


bool Mantids29::Helpers::File::isSensitiveConfigPermissionInsecure(const std::string &configFilePath, bool * isInsecure)
{
    *isInsecure = false;

#ifndef _WIN32
    struct stat fileInfo;
    if ( access(configFilePath.c_str(), R_OK ) || stat(configFilePath.c_str(), &fileInfo) != 0) {
        return false;
    }

    // Check if the file permissions are different from 0600
    *isInsecure = (fileInfo.st_mode & 0777) != 0600;

    // return if succeed...
    return true;
#else
    // TODO: win32 check for secure file permissions...
    return true;
#endif
}

bool Mantids29::Helpers::File::fixSensitiveConfigPermission(const std::string &configFilePath)
{
#ifndef _WIN32
    // Change the value to 0600
    if (chmod(configFilePath.c_str(), 0600) != 0)
    {
        return false;
    }
    return true;
#else
    // TODO: win32 fix for secure file permissions...
    return true;
#endif
}
