#ifndef FILE_H
#define FILE_H

#include <string>

namespace Mantids29 { namespace Helpers {

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
};


}};

#endif // FILE_H
