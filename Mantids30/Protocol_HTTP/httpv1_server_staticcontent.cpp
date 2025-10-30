#include "httpv1_server.h"

using namespace Mantids30::Network::Protocols;

using namespace std;

void HTTP::HTTPv1_Server::setStaticContentElements(const std::map<std::string, std::shared_ptr<Mantids30::Memory::Containers::B_MEM>> &value)
{
    m_staticContentElements = value;
}

bool HTTP::HTTPv1_Server::verifyStaticContentExistence(const string &path)
{
    return !(m_staticContentElements.find(path) == m_staticContentElements.end());
}

void HTTP::HTTPv1_Server::addStaticContent(const string &path, std::shared_ptr<Memory::Containers::B_MEM> contentElement)
{
    m_staticContentElements[path] = contentElement;
}
