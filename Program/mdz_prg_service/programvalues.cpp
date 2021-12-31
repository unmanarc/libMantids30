#include "programvalues.h"
#include <vector>
#include <sstream>
#include <iostream>

using namespace Mantids::Application::Values;

ProgramValues::ProgramValues()
{

}

void ProgramValues::initProgramName(const std::string &value)
{
    std::vector<std::string> paths;

    // Split into /.
    std::istringstream f(value);
    std::string s;
    while (getline(f, s, '/')) paths.push_back(s);

    programName = !paths.size()? "unknownprogram" : paths.at(paths.size()-1);
    description = programName;
    daemonName = programName;
}

std::string ProgramValues::getProgramName() const
{
    return programName;
}

void ProgramValues::setProgramName(const std::string &value)
{
    programName = value;
}

std::string ProgramValues::getAuthor() const
{
    return author;
}

void ProgramValues::setAuthor(const std::string &value)
{
    author = value;
}

std::string ProgramValues::getEmail() const
{
    return email;
}

void ProgramValues::setEmail(const std::string &value)
{
    email = value;
}

std::string ProgramValues::getVersion() const
{
    return version;
}

void ProgramValues::setVersion(const std::string &value)
{
    version = value;
}

void ProgramValues::setVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string &subText)
{
    setVersion(std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(subminor) +  (subText.empty()? "" : (" " + subText))  );
}

std::string ProgramValues::getDescription() const
{
    return description;
}

void ProgramValues::setDescription(const std::string &value)
{
    description = value;
}

std::string ProgramValues::getDaemonName() const
{
    return daemonName;
}

void ProgramValues::setDaemonName(const std::string &value)
{
    daemonName = value;
}

std::string ProgramValues::getLicense() const
{
    return license;
}

void ProgramValues::setLicense(const std::string &value)
{
    license = value;
}

