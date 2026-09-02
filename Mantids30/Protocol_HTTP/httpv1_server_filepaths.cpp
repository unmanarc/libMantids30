#include "httpv1_server.h"
#include <sys/stat.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace boost;
using namespace boost::algorithm;

using namespace Mantids30::Network::Protocol;
using namespace Mantids30::Network;
using namespace Mantids30;

using namespace std;

bool HTTP::HTTPv1_Server::resolveLocalFilePathFromURI2(string defaultWebRootWithEndingSlash, const std::list<std::pair<std::string, std::string>> &overlappedDirectories,
                                                       LocalRequestedFileInfo *outFileInfo, const std::string &defaultFileToAppend, const bool &preventMappingExecutables)
{
    if (!outFileInfo)
    {
        throw std::runtime_error(std::string(__func__) + std::string(" Should be called with info object... Aborting..."));
    }

    outFileInfo->reset();

    std::string requestedURI = clientRequest.getURI();

    std::string (*resolveDirPathWithSlashAtEnd)(const std::string &) = [](const std::string &serverDirectoryPath) -> std::string
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
        [[nodiscard]] bool detectPathTraversal() const
        {
            return (fileSystemRealPath.size() < serverWebRootWithEndingSlash.size()                                                       // outside dir?
                    || memcmp(serverWebRootWithEndingSlash.c_str(), fileSystemRealPath.c_str(), serverWebRootWithEndingSlash.size()) != 0 // not matching?
            );
        }

        [[nodiscard]] std::string getRelativePath() const
        {
            // Eg. Relative Path: /assets/style.css
            return urlPathPrefix + fileSystemRealPath.substr(serverWebRootWithEndingSlash.size());
        }

        std::string fileSystemRealPath;           // Eg. /var/assets/style.css or /var/assets/js/ for dir
        std::string serverWebRootWithEndingSlash; // Eg. /var/assets/
        std::string urlPathPrefix;                // Eg. /assets/, /
        struct stat fileStats{};
    };

    std::vector<RequestedOverlapInfo> detectedPotentialOverlaps;
    std::string requestURIWithoutLeadingSlash = (requestedURI.size() <= 1) ? "/" : requestedURI.substr(1);
    std::string requestURIWithDefaultFile = requestURIWithoutLeadingSlash + defaultFileToAppend;
    detectedPotentialOverlaps.push_back({defaultWebRootWithEndingSlash + requestURIWithDefaultFile, defaultWebRootWithEndingSlash, "/"}); // Add the original path

    // Add overlapped directories if they match the requested URI
    for (const auto &overlappedDirectory : overlappedDirectories)
    {
        // All the overlapped dirs should end with /
        if (overlappedDirectory.first.empty() || overlappedDirectory.second.empty())
        {
            continue;
        }

        if (boost::starts_with(requestedURI, overlappedDirectory.first))
        {
            std::string overlappedWebRootWithEndingSlash = resolveDirPathWithSlashAtEnd(overlappedDirectory.second);
            if (overlappedWebRootWithEndingSlash.empty())
            {
                continue;
            }

            // Compute the full path with overlapped directory
            RequestedOverlapInfo oInfo = {overlappedWebRootWithEndingSlash + requestURIWithDefaultFile.substr(overlappedDirectory.first.size() - 1),
                                          overlappedWebRootWithEndingSlash,
                                          overlappedDirectory.first};
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
            serverResponse.cacheControl.optionNoCache = true;
            serverResponse.cacheControl.optionNoStore = false;
            serverResponse.cacheControl.optionMustRevalidate = true;
            serverResponse.cacheControl.maxAge = 3600*6;
            serverResponse.cacheControl.optionImmutable = false;

            outFileInfo->relativePath = requestedURI + defaultFileToAppend;

            detectContentTypeFromFilePath(outFileInfo->relativePath);

            outFileInfo->fullPath = "MEM:" + outFileInfo->relativePath;

            auto staticContentElement =m_staticContentElements[outFileInfo->relativePath];
            serverResponse.setContentDataStreamer(staticContentElement.getData());

            // Get ETag for in-memory static content
            auto staticContent = m_staticContentElements[outFileInfo->relativePath];
            serverResponse.etag.setValue(staticContentElement.getETag());
            serverResponse.etag.setWeak(false);

            return true;
        }
        else
        {
            bool pathFound = false;

            for (const RequestedOverlapInfo &rpInfo : detectedPotentialOverlaps)
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
                    if ((outFileInfo->isDirectory = S_ISDIR(selectedOverlap.fileStats.st_mode)) == true)
                    {
                        selectedOverlap.fileSystemRealPath += (selectedOverlap.fileSystemRealPath.back() == SLASHB ? "" : std::string(SLASH));
                    }

                    // Path OK, continue.

                    // Check for transversal access hacking attempts...
                    if (selectedOverlap.detectPathTraversal())
                    {
                        outFileInfo->isTransversal = true;
                        return false;
                    }

                    pathFound = true;
                    break;
                }
            }

            if (!pathFound)
            {
                return false;
            }
        }
    }

    // No transversal detected at this point.

    // Check if it's a directory...

    if (outFileInfo->isDirectory)
    {
        outFileInfo->exists = true;

        // Don't get directories when we are appending something.
        if (!defaultFileToAppend.empty())
        {
            return false;
        }

        outFileInfo->fullPath = selectedOverlap.fileSystemRealPath;
        outFileInfo->relativePath = selectedOverlap.getRelativePath();

        // Do we have access?:
        return !access(outFileInfo->fullPath.c_str(), R_OK);
    }
    else if (S_ISREG(selectedOverlap.fileStats.st_mode)) // Check if it's a regular file
    {
        outFileInfo->exists = true;

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
            outFileInfo->fullPath = selectedOverlap.fileSystemRealPath;
            outFileInfo->relativePath = selectedOverlap.getRelativePath();
            outFileInfo->isExecutable = true;
            outFileInfo->exists = true;
            return true;
        }
        else
        {
            std::shared_ptr<Mantids30::Memory::Containers::B_MMAP> fileMemoryMap = std::make_shared<Mantids30::Memory::Containers::B_MMAP>();
            if (fileMemoryMap->referenceFile(selectedOverlap.fileSystemRealPath, true, false))
            {
                // File Found / Readable.
                outFileInfo->fullPath = selectedOverlap.fileSystemRealPath;
                outFileInfo->relativePath = selectedOverlap.getRelativePath();
                serverResponse.setContentDataStreamer(fileMemoryMap);
                detectContentTypeFromFilePath(outFileInfo->relativePath);

                HTTP::Date fileModificationDate;
#ifdef _WIN32
                fileModificationDate.setUnixTime(selectedPathInfo.fileStats.st_mtime);
#else
                fileModificationDate.setUnixTime(selectedOverlap.fileStats.st_mtim.tv_sec);
#endif
                if (serverResponse.includeDate)
                {
                    serverResponse.headers.replace("Last-Modified", fileModificationDate.toString());
                }

                serverResponse.cacheControl.optionNoCache = true;
                serverResponse.cacheControl.optionNoStore = false;
                serverResponse.cacheControl.optionMustRevalidate = true;
                serverResponse.cacheControl.maxAge = 3600*6;
                serverResponse.cacheControl.optionImmutable = false;

                // Generate ETag based on size and modification time (portable across platforms)
#ifdef _WIN32
                serverResponse.etag.setValue(std::to_string(selectedOverlap.fileStats.st_size) + "-" + std::to_string(selectedOverlap.fileStats.st_mtime));
#else
                serverResponse.etag.setValue(std::to_string(selectedOverlap.fileStats.st_ino) + "-" + std::to_string(selectedOverlap.fileStats.st_size) + "-"
                                            + std::to_string(selectedOverlap.fileStats.st_mtim.tv_sec));
#endif
                serverResponse.etag.setWeak(false);

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
    {
        throw std::runtime_error(std::string(__func__) + std::string(" Should be called with info object... Aborting..."));
    }

    info->reset();

    {
        char *cServerDir;
        // Check Server Dir Real Path:
        if ((cServerDir = realpath((sServerDir).c_str(), nullptr)) == nullptr)
        {
            return false;
        }

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
            return false;
            /*if (errno == ENOENT)
            {
                // Non-existent file.
                return false; // or handle the error as needed
            }
            else
            {
                // Other error occurred.
                return false; // or handle the error as needed
            }*/
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
    {
        throw std::runtime_error(std::string(__func__) + std::string(" Should be called with info object... Aborting..."));
    }

    info->reset();

    {
        char *cServerDir;
        // Check Server Dir Real Path:
        if ((cServerDir = realpath((sServerDir).c_str(), nullptr)) == nullptr)
        {
            return false;
        }

        sServerDir = cServerDir;

        // Put a slash at the end of the server dir resource...
        sServerDir += (sServerDir.back() == SLASHB ? "" : std::string(SLASH));

        free(cServerDir);
    }

    // Compute the requested path:
    string sFullRequestedPath = sServerDir                           // Put the current server dir...
                                + (uri.empty() ? "" : uri.substr(1)) // Put the Request URI (without the first character / slash)
        ;                                                            // Append option...

    struct stat stats{};

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
            {
                sFullComputedRealPath += (sFullComputedRealPath.back() == SLASHB ? "" : std::string(SLASH));
            }
        }
        else
        { // Non-Existant File.
            return false;
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
