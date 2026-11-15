#pragma once

#include <string>

namespace Mantids30::Helpers {

/**
 * Provides utility functions for working with files.
 */
class File
{
public:
    /**
     * Loads the contents of a file into a string.
     *
     * @param filePath The path to the file to load.
     * @param ok A pointer to a boolean that will be set to true if the file was loaded successfully, false otherwise.
     *
     * @return The contents of the file as a string.
     */
    static std::string loadFileIntoString(const std::string &filePath, bool *ok = nullptr);

    /**
     * Writes a string to a file.
     *
     * @param filePath The path to the file to write.
     * @param content The content to write to the file.
     *
     * @return True if the content was successfully written to the file, false otherwise.
     */
    static bool writeStringToFile(const std::string &filePath, const std::string &content);

    /**
     * @brief Detects if the provided configuration file has insecure permissions
     *
     * @param configFilePath The path to the configuration file to check
     * @param isInsecure A pointer to a boolean variable that will be set to true if the file's permissions are insecure, and false otherwise
     * @return true if the function succeeds, false if you don't have access to this file
     */
    static bool isSensitiveConfigPermissionInsecure(const std::string &configFilePath, bool *isInsecure);

    /**
     * @brief Fixes the insecure permissions of the provided configuration file
     *
     * @param configFilePath The path to the configuration file to fix
     * @return true if the function succeeds, false otherwise
     */
    static bool fixSensitiveConfigPermission(const std::string &configFilePath);
};

} // namespace Mantids30::Helpers
