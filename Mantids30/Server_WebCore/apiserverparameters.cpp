#include "apiserverparameters.h"
#include <memory>

using namespace Mantids30::Network::Servers::Web;


APIServerParameters::~APIServerParameters()
{
    for (const auto & i : m_memToBeFreed)
    {
        free(i);
    }
}

void APIServerParameters::setSoftwareVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string &subText)
{
    softwareVersion = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(subminor) + (subText.empty() ? "" : (" " + subText));
}

bool APIServerParameters::isDocumentRootPathAccesible() const
{
    return !access(m_documentRootPath.c_str(),R_OK);
}

bool APIServerParameters::setDocumentRootPath(const std::string &value)
{
    // Check if the provided path has read access.
    if (access(value.c_str(), R_OK))
        return false;

    // Set the local path for resources.
    m_documentRootPath = value;

    // If the provided path is empty, reset the path and return true.
    if (value.empty())
    {
        m_documentRootPath = "";
        return true;
    }

    // Resolve the full absolute path and set `documentRootPath` if successful.
    char *cFullPath = realpath(value.c_str(), nullptr);
    if (cFullPath)
    {
        m_documentRootPath = cFullPath;
        free(cFullPath); // Free the allocated memory after usage.
    }
    else
    {
        // If `realpath` fails, keep the original path.
        m_documentRootPath = value;
    }

    // If automatic loading of resources filters is enabled:
    if (autoloadResourcesFilter)
    {
        // Construct the path to the `resources.conf` file.
        std::string resourceFilterPath = m_documentRootPath + "/resources.conf";

        // Check if the `resources.conf` file has read access.
        if (!access(resourceFilterPath.c_str(), R_OK))
        {
            // Create a new `ResourcesFilter` object and attempt to load filters from the file.
            std::shared_ptr<API::Web::ResourcesFilter> rf = std::make_shared<API::Web::ResourcesFilter>();
            if (rf->loadFiltersFromFile(resourceFilterPath))
            {
                // If an existing filter is set, delete it before replacing.
/*                if (resourceFilter)
                    delete resourceFilter;*/

                // Assign the new resources filter. (if there is another resource filter, the first one is destroyed)
                resourceFilter = rf;
            }
            // If loading fails, delete the newly created filter object (automatically by the shared pointer)
        }
    }

    // Return true to indicate that the document root path was set successfully.
    return true;
}

void APIServerParameters::addStaticContentElement(const std::string &path, const std::string &content)
{
    std::lock_guard<std::mutex> lck(m_internalContentMutex);
    // TODO: update.... (when no http clients running)
    if (m_staticContentElements.find(path) == m_staticContentElements.end())
    {
        char *xmem = (char *) malloc(content.size() + 1);
        xmem[content.size()] = 0;
        memcpy(xmem, content.c_str(), content.size());
        m_staticContentElements[path] = std::make_shared<Mantids30::Memory::Containers::B_MEM>(xmem, content.size());
        m_memToBeFreed.push_back(xmem);
    }
}

std::map<std::string, std::shared_ptr<Mantids30::Memory::Containers::B_MEM> > APIServerParameters::getStaticContentElements()
{
    std::lock_guard<std::mutex> lck(m_internalContentMutex);
    return m_staticContentElements;
}

std::string APIServerParameters::getDocumentRootPath() const
{
    return m_documentRootPath;
}


