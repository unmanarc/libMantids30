#include "programvalues.h"
#include <vector>
#include <sstream>
#include <iostream>

using namespace Mantids29::Application::Values;

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

    m_programName = !paths.size()? "unknownprogram" : paths.at(paths.size()-1);
    m_softwareDescription = m_programName;
    m_daemonName = m_programName;
}


void ProgramValues::addAuthor(const Author &author)
{
    m_authors.push_back(author);
}

std::list<Author> ProgramValues::getAuthors()
{
    return m_authors;
}

std::string ProgramValues::getVersion() const
{
    return m_version;
}

void ProgramValues::setVersion(const std::string &value)
{
    m_version = value;
}

void ProgramValues::setVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string &subText)
{
    setVersion(std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(subminor) +  (subText.empty()? "" : (" " + subText))  );
}

std::string ProgramValues::getDaemonName() const
{
    return m_daemonName;
}

void ProgramValues::setDaemonName(const std::string &value)
{
    m_daemonName = value;
}
