#ifndef FILE_H
#define FILE_H

#include <string>

namespace Mantids { namespace Helpers {


class File
{
public:
    static std::string loadFileIntoString(const std::string &file, bool * ok = nullptr);
    static bool loadStringIntoFile(const std::string & file, const std::string &content);
};

}};

#endif // FILE_H
