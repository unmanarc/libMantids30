#include "file.h"

#include <fstream>

std::string Mantids::Helpers::File::loadFileIntoString(const std::string &file, bool *ok)
{
    if (ok)
        *ok = false;

    std::ifstream infile(file);
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

bool Mantids::Helpers::File::loadStringIntoFile(const std::string &file, const std::string &content)
{
    std::ofstream outfile;
    outfile.open(file, std::ios_base::out);
    if (outfile.is_open())
    {
        outfile  << content;
        outfile.close();
        return true;
    }
    return false;
}
