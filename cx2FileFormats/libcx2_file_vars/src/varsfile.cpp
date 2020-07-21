#include "varsfile.h"
#include <fstream>
#include <sstream>

using namespace CX2::Files::Vars;

File::File(const std::string &filePath)
{
    this->filePath = filePath;
    lastError = NO_ERROR;
}

bool File::load()
{
    std::ifstream varsfile(filePath);
    if (!varsfile.is_open())
    {
        lastError = CANT_OPEN_FILE;
        return false;
    }
    std::string line;
    bool ok;
    while (std::getline(varsfile, line, '\n'))
    {
        std::pair<std::string, std::string> lineVars = getLineVars(line, &ok);
        if (!ok)
        {
            lastError = INVALID_FILE_FORMAT;
            varsfile.close();
            return false;
        }
        vars.insert(lineVars);
    }
    varsfile.close();
    return true;
}

bool File::save()
{
    std::ofstream varsfile (filePath, std::ofstream::out);
    if (!varsfile.is_open())
    {
        lastError = CANT_OPEN_FILE;
        return false;
    }

    bool ok;
    for ( const auto & i : vars )
    {
        std::string value = getLineFromVars(i,&ok);
        if (!ok)
        {
            lastError = INVALID_VARNAME_FORMAT;
            varsfile.close();
            return false;
        }
        varsfile << value;
    }
    varsfile.close();
    return true;
}

void File::setVar(const std::string &varName, const std::string &varValue)
{
    vars.erase(varName);
    addVar(varName,varValue);
}

void File::addVar(const std::string &varName, const std::string &varValue)
{
    vars.insert(std::make_pair(varName,varValue));
}

std::list<std::string> File::getVarValues(const std::string &varName)
{
    std::list<std::string> r;
    auto ret = vars.equal_range(varName);
    for (std::multimap<std::string,std::string>::iterator it=ret.first; it!=ret.second; ++it)
    {
          r.push_back(it->second);
    }
    return r;
}

std::string File::getVarValue(const std::string &varName, bool * found)
{
    auto i = vars.find(varName);

    if (i == vars.end())
    {
        if (found) *found = false;
        return "";
    }

    if (found) *found = true;
    return i->second;
}

std::pair<std::string, std::string> File::getLineVars(const std::string &line, bool *ok)
{
    std::string varName,varValue;
    std::stringstream sStrValues(line);

    if (!std::getline(sStrValues, varName, ':'))
    {
        if (ok) *ok = false;
        return std::make_pair("","");
    }

    if (line.size() <= varName.size())
    {
        if (ok) *ok = false;
        return std::make_pair("","");
    }

    varValue = line.substr(0,varName.size()+1);

    if (ok) *ok = true;
    return std::make_pair(varName,varValue);
}

std::string File::getLineFromVars(const std::pair<std::string, std::string> &vars, bool *ok)
{
    if (vars.first.find(":")!=std::string::npos)
    {
        if (ok) *ok = false;
        return "";
    }
    if (ok) *ok = true;
    return vars.first + ":" + vars.second;
}

eFileError File::getLastError() const
{
    return lastError;
}

std::multimap<std::string, std::string> File::getVars() const
{
    return vars;
}

std::map<std::string, std::string> File::getVarsMap() const
{
    std::map<std::string, std::string> r;
    for (auto & i : vars)
    {
        r[i.first]=i.second;
    }
    return r;
}
