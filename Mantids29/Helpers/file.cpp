#include "file.h"

#include <fstream>

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
