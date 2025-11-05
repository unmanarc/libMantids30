#include "httpv1_server.h"
#include <sys/stat.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace boost;
using namespace boost::algorithm;


using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

using namespace std;



bool HTTP::HTTPv1_Server::resolveLocalFilePathFromURI2(string defaultWebRootWithEndingSlash, const std::list<std::pair<std::string, std::string>> &overlappedDirectories, LocalRequestedFileInfo *info,
                                                   const std::string &defaultFileToAppend, const bool &preventMappingExecutables)
{
    if (!info)
        throw std::runtime_error(std::string(__func__) + std::string(" Should be called with info object... Aborting..."));

    info->reset();

    std::string requestedURI = clientRequest.getURI();

    auto resolveDirPathWithSlashAtEnd = [](const std::string &serverDirectoryPath) -> std::string
    {
        // Use unique_ptr for automatic memory management
        std::unique_ptr<char, decltype(&free)> resolvedPathUniquePtr(realpath(serverDirectoryPath.c_str(), nullptr), &free);
        if (!resolvedPathUniquePtr)
        {
            return "";
        }
        std::string resolvedPath(resolvedPathUniquePtr.get());
        // Put a slash at the end of the server dir resource...
        if (!resolvedPath.empty() && resolvedPath.back() != SLASHB)
        {
            resolvedPath += SLASH;
        }
        return resolvedPath;
    };

    defaultWebRootWithEndingSlash = resolveDirPathWithSlashAtEnd(defaultWebRootWithEndingSlash);
    if (defaultWebRootWithEndingSlash.empty())
    {
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create list of potential paths to check

    struct RequestedOverlapInfo
    {
        bool detectPathTraversal()
        {
            if (fileSystemRealPath.size() < serverWebRootWithEndingSlash.size()                                                    // outside dir?
                || memcmp(serverWebRootWithEndingSlash.c_str(), fileSystemRealPath.c_str(), serverWebRootWithEndingSlash.size()) != 0 // not matching?
                )
            {
                return true;
            }
            return false;
        }

        std::string getRelativePath()
        {
            // Eg. Relative Path: /assets/style.css
            return urlPathPrefix + fileSystemRealPath.substr(serverWebRootWithEndingSlash.size());
        }

        std::string fileSystemRealPath; // Eg. /var/assets/style.css or /var/assets/js/ for dir
        std::string serverWebRootWithEndingSlash; // Eg. /var/assets/
        std::string urlPathPrefix; // Eg. /assets/, /
        struct stat fileStats;
    };

    std::vector<RequestedOverlapInfo> detectedPotentialOverlaps;
    std::string requestURIWithoutLeadingSlash = (requestedURI.size()<=1) ? "/" : requestedURI.substr(1);
    std::string requestURIWithDefaultFile = requestURIWithoutLeadingSlash + defaultFileToAppend;
    detectedPotentialOverlaps.push_back({defaultWebRootWithEndingSlash + requestURIWithDefaultFile, defaultWebRootWithEndingSlash, "/"}); // Add the original path

    // Add overlapped directories if they match the requested URI
    for (const auto &overlappedDirectory : overlappedDirectories)
    {
        // All the overlapped dirs should end with /
        if (overlappedDirectory.first.empty() || overlappedDirectory.second.empty())
            continue;

        if (boost::starts_with(requestedURI, overlappedDirectory.first))
        {
            std::string overlappedWebRootWithEndingSlash = resolveDirPathWithSlashAtEnd(overlappedDirectory.second);
            if (overlappedWebRootWithEndingSlash.empty())
            {
                continue;
            }

            // Compute the full path with overlapped directory
            RequestedOverlapInfo oInfo = {overlappedWebRootWithEndingSlash + requestURIWithDefaultFile.substr(overlappedDirectory.first.size() - 1), overlappedWebRootWithEndingSlash, overlappedDirectory.first};
            detectedPotentialOverlaps.push_back(oInfo);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // Compute the full requested path:
    RequestedOverlapInfo selectedOverlap;
    {
        char *cFullPath = nullptr;
        if (m_staticContentElements.find(requestURIWithDefaultFile) != m_staticContentElements.end())
        {
            // STATIC CONTENT:
            serverResponse.cacheControl.optionNoCache = false;
            serverResponse.cacheControl.optionNoStore = false;
            serverResponse.cacheControl.optionMustRevalidate = false;
            serverResponse.cacheControl.maxAge = 3600;
            serverResponse.cacheControl.optionImmutable = true;

            info->relativePath = requestedURI + defaultFileToAppend;

            detectContentTypeFromFilePath(info->relativePath);

            info->fullPath = "MEM:" + info->relativePath;

            serverResponse.setDataStreamer(m_staticContentElements[info->relativePath]);
            return true;
        }
        else
        {
            bool pathFound = false;

            for (const auto &rpInfo : detectedPotentialOverlaps)
            {
                selectedOverlap = rpInfo;
                if ((cFullPath = realpath(rpInfo.fileSystemRealPath.c_str(), nullptr)) != nullptr)
                {
                    // Compute the full path..
                    selectedOverlap.fileSystemRealPath = cFullPath;
                    free(cFullPath);

                    // Check file properties...
                    stat(selectedOverlap.fileSystemRealPath.c_str(), &selectedOverlap.fileStats);

                    // Put a slash at the end of the computed dir resource (when dir)...
                    if ((info->isDirectory = S_ISDIR(selectedOverlap.fileStats.st_mode)) == true)
                        selectedOverlap.fileSystemRealPath += (selectedOverlap.fileSystemRealPath.back() == SLASHB ? "" : std::string(SLASH));

                    // Path OK, continue.

                    // Check for transversal access hacking attempts...
                    if (selectedOverlap.detectPathTraversal())
                    {
                        info->isTransversal = true;
                        return false;
                    }

                    pathFound = true;
                    break;
                }
            }

            if (!pathFound)
                return false;
        }
    }

    // No transversal detected at this point.

    // Check if it's a directory...

    if (info->isDirectory == true)
    {
        info->exists = true;

        // Don't get directories when we are appending something.
        if (!defaultFileToAppend.empty())
            return false;

        info->fullPath = selectedOverlap.fileSystemRealPath;
        info->relativePath = selectedOverlap.getRelativePath();

        // Do we have access?:
        return !access(info->fullPath.c_str(), R_OK);
    }
    else if (S_ISREG(selectedOverlap.fileStats.st_mode) == true) // Check if it's a regular file
    {
        info->exists = true;

        if (preventMappingExecutables &&
#ifndef _WIN32
            !access(selectedOverlap.fileSystemRealPath.c_str(), X_OK)
#else
            (boost::iends_with(requestedPathInfo.fsPath, ".exe") || boost::iends_with(requestedPathInfo.fsPath, ".bat") || boost::iends_with(requestedPathInfo.fsPath, ".com"))
#endif
            )
        {
            // file is executable... don't map, and the most important: don't create cache in the browser...
            // Very useful for CGI-like implementations...
            info->fullPath = selectedOverlap.fileSystemRealPath;
            info->relativePath = selectedOverlap.getRelativePath();
            info->isExecutable = true;
            info->exists = true;
            return true;
        }
        else
        {
            std::shared_ptr<Mantids30::Memory::Containers::B_MMAP> fileMemoryMap = std::make_shared<Mantids30::Memory::Containers::B_MMAP>();
            if (fileMemoryMap->referenceFile(selectedOverlap.fileSystemRealPath.c_str(), true, false))
            {
                // File Found / Readable.
                info->fullPath = selectedOverlap.fileSystemRealPath;
                info->relativePath = selectedOverlap.getRelativePath();
                serverResponse.setDataStreamer(fileMemoryMap);
                detectContentTypeFromFilePath(info->relativePath);

                HTTP::Date fileModificationDate;
#ifdef _WIN32
                fileModificationDate.setUnixTime(selectedPathInfo.fileStats.st_mtime);
#else
                fileModificationDate.setUnixTime(selectedOverlap.fileStats.st_mtim.tv_sec);
#endif
                if (serverResponse.includeDate)
                    serverResponse.headers.add("Last-Modified", fileModificationDate.toString());

                serverResponse.cacheControl.optionNoCache = false;
                serverResponse.cacheControl.optionNoStore = false;
                serverResponse.cacheControl.optionMustRevalidate = false;
                serverResponse.cacheControl.maxAge = 3600;
                serverResponse.cacheControl.optionImmutable = true;
                return true;
            }
            return false;
        }
    }
    else
    {
        // Special files...
        return false;
    }
}

bool HTTP::HTTPv1_Server::resolveLocalFilePathFromURI0NE(const std::string &uri, std::string sServerDir, LocalRequestedFileInfo *info)
{
    if (!info)
        throw std::runtime_error(std::string(__func__) + std::string(" Should be called with info object... Aborting..."));

    info->reset();

    {
        char *cServerDir;
        // Check Server Dir Real Path:
        if ((cServerDir = realpath((sServerDir).c_str(), nullptr)) == nullptr)
            return false;

        sServerDir = cServerDir;

        // Put a slash at the end of the server dir resource...
        sServerDir += (sServerDir.back() == SLASHB ? "" : std::string(SLASH));

        free(cServerDir);
    }

    // Compute the requested path:
    string sFullRequestedPath = sServerDir                           // Put the current server dir...
                                + (uri.empty() ? "" : uri.substr(1)) // Put the Request URI (without the first character / slash)
        ;                                                            // Append option...

    // Compute the full requested path:
    std::string sFullComputedRealPath;
    {
        char cRealPath[PATH_MAX];
        if (realpath(sFullRequestedPath.c_str(), cRealPath) == nullptr)
        {
            if (errno == ENOENT)
            {
                // Non-existent file.
                return false; // or handle the error as needed
            }
            else
            {
                // Other error occurred.
                return false; // or handle the error as needed
            }
        }
        else
        {
            sFullComputedRealPath = cRealPath;
        }
    }

    // Check for transversal access hacking attempts...
    if (sFullComputedRealPath.size() < sServerDir.size() || memcmp(sServerDir.c_str(), sFullComputedRealPath.c_str(), sServerDir.size()) != 0)
    {
        info->isTransversal = true;
        return false;
    }

    // No transversal detected at this point.
    info->fullPath = sFullComputedRealPath;
    info->relativePath = sFullComputedRealPath.c_str() + (sServerDir.size() - 1);

    return true;
}

bool HTTP::HTTPv1_Server::resolveLocalFilePathFromURI0E(const std::string &uri, std::string sServerDir, LocalRequestedFileInfo *info)
{
    if (!info)
        throw std::runtime_error(std::string(__func__) + std::string(" Should be called with info object... Aborting..."));

    info->reset();

    {
        char *cServerDir;
        // Check Server Dir Real Path:
        if ((cServerDir = realpath((sServerDir).c_str(), nullptr)) == nullptr)
            return false;

        sServerDir = cServerDir;

        // Put a slash at the end of the server dir resource...
        sServerDir += (sServerDir.back() == SLASHB ? "" : std::string(SLASH));

        free(cServerDir);
    }

    // Compute the requested path:
    string sFullRequestedPath = sServerDir                           // Put the current server dir...
                                + (uri.empty() ? "" : uri.substr(1)) // Put the Request URI (without the first character / slash)
        ;                                                            // Append option...

    struct stat stats;

    // Compute the full requested path:
    std::string sFullComputedRealPath;
    {
        char cRealPath[PATH_MAX + 2];
        if (realpath(sFullRequestedPath.c_str(), cRealPath))
        {
            sFullComputedRealPath = cRealPath;

            // Check file properties...
            stat(sFullComputedRealPath.c_str(), &stats);
            // Put a slash at the end of the computed dir resource (when dir)...
            if ((info->isDirectory = S_ISDIR(stats.st_mode)) == true)
                sFullComputedRealPath += (sFullComputedRealPath.back() == SLASHB ? "" : std::string(SLASH));
        }
        else // Non-Existant File.
            return false;
    }

    // Check for transversal access hacking attempts...
    if (sFullComputedRealPath.size() < sServerDir.size() || memcmp(sServerDir.c_str(), sFullComputedRealPath.c_str(), sServerDir.size()) != 0)
    {
        info->isTransversal = true;
        return false;
    }

    // No transversal detected at this point.
    info->fullPath = sFullComputedRealPath;
    info->relativePath = sFullComputedRealPath.c_str() + (sServerDir.size() - 1);

    return true;
}

