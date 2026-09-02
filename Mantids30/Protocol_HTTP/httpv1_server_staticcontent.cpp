#include "httpv1_server.h"
#include "Mantids30/Helpers/crypto.h"

#include <ctime>

using namespace Mantids30::Network::Protocol;

using namespace std;

// StaticContentElement implementation
void HTTP::HTTPv1_Server::StaticContentElement::setData(const std::shared_ptr<Mantids30::Memory::Containers::B_MEM>& data, const time_t &lastModifiedTime)
{
    this->data = data;
    this->lastModifiedTime = lastModifiedTime;

    // Calculate ETAG: SHA256(content + lastModifiedTime)
    if (data && !data->isNull())
    {
        // Get content as string
        auto contentStr = data->toString();
        if (contentStr)
        {
            // Append lastModifiedTime to the hash input
            std::string hashInput = *contentStr + std::to_string(lastModifiedTime);

            // Calculate SHA256 hash
            std::string sha256Hash = Mantids30::Helpers::Crypto::calcSHA256(hashInput);

            // Use first 16 hex characters (8 bytes) of the SHA256 hash for the ETAG
            // SHA256 returns a hex string, so take first 16 chars
            this->etagHash = sha256Hash.substr(0, 16);
        }
        else
        {
            this->etagHash = "empty";
        }
    }
    else
    {
        this->etagHash = "empty";
    }
}

std::string HTTP::HTTPv1_Server::StaticContentElement::getETag()
{
    return this->etagHash;
}

// HTTPv1_Server static content methods
void HTTP::HTTPv1_Server::setStaticContentElements(const std::map<std::string, StaticContentElement> &value)
{
    m_staticContentElements = value;
}

bool HTTP::HTTPv1_Server::verifyStaticContentExistence(const string &path)
{
    return m_staticContentElements.find(path) != m_staticContentElements.end();
}

void HTTP::HTTPv1_Server::addStaticContent(const string &path, const StaticContentElement &contentElement)
{
    m_staticContentElements[path] = contentElement;
}
