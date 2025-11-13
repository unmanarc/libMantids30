#include "httpv1_server.h"
#include <boost/algorithm/string/case_conv.hpp>

using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

using namespace std;

void HTTP::HTTPv1_Server::setMimeTypeForFileExtension(const string &ext, const string &type)
{
    m_mimeTypes[ext] = type;
}

bool HTTP::HTTPv1_Server::detectContentTypeFromFilePath(const string &sFilePath)
{
    const char *cFileExtension = strrchr(sFilePath.c_str(), '.');

    if (!cFileExtension || cFileExtension[1] == 0)
        return false;

    m_currentFileExtension = boost::to_lower_copy(std::string(cFileExtension));

    if (m_mimeTypes.find(m_currentFileExtension) != m_mimeTypes.end())
    {
        serverResponse.setContentType(m_mimeTypes[m_currentFileExtension], true);
        return true;
    }
    serverResponse.setContentType("", false);
    return false;
}

std::string HTTP::HTTPv1_Server::getCurrentFileExtension() const
{
    return m_currentFileExtension;
}



void HTTP::HTTPv1_Server::loadDefaultMIMETypes()
{
    // Default Mime Types (ref: https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types)
    m_mimeTypes[".aac"] = "audio/aac";
    m_mimeTypes[".abw"] = "application/x-abiword";
    m_mimeTypes[".arc"] = "application/x-freearc";
    m_mimeTypes[".avi"] = "video/x-msvideo";
    m_mimeTypes[".azw"] = "application/vnd.amazon.ebook";
    m_mimeTypes[".bin"] = "application/octet-stream";
    m_mimeTypes[".bmp"] = "image/bmp";
    m_mimeTypes[".bz"] = "application/x-bzip";
    m_mimeTypes[".bz2"] = "application/x-bzip2";
    m_mimeTypes[".csh"] = "application/x-csh";
    m_mimeTypes[".css"] = "text/css";
    m_mimeTypes[".csv"] = "text/csv";
    m_mimeTypes[".doc"] = "application/msword";
    m_mimeTypes[".docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    m_mimeTypes[".eot"] = "application/vnd.ms-fontobject";
    m_mimeTypes[".epub"] = "application/epub+zip";
    m_mimeTypes[".gz"] = "application/gzip";
    m_mimeTypes[".gif"] = "image/gif";
    m_mimeTypes[".htm"] = "text/html";
    m_mimeTypes[".html"] = "text/html";
    m_mimeTypes[".iso"] = "application/octet-stream";
    m_mimeTypes[".ico"] = "image/vnd.microsoft.icon";
    m_mimeTypes[".ics"] = "text/calendar";
    m_mimeTypes[".jar"] = "application/java-archive";
    m_mimeTypes[".jpeg"] = "image/jpeg";
    m_mimeTypes[".jpg"] = "image/jpeg";
    m_mimeTypes[".js"] = "application/javascript";
    m_mimeTypes[".json"] = "application/json";
    m_mimeTypes[".jsonld"] = "application/ld+json";
    m_mimeTypes[".mid"] = "audio/midi";
    m_mimeTypes[".midi"] = "audio/x-midi";
    m_mimeTypes[".mjs"] = "text/javascript";
    m_mimeTypes[".mp3"] = "audio/mpeg";
    m_mimeTypes[".mp4"] = "video/mp4";
    m_mimeTypes[".mpeg"] = "video/mpeg";
    m_mimeTypes[".mpg"] = "video/mpeg";
    m_mimeTypes[".mpkg"] = "application/vnd.apple.installer+xml";
    m_mimeTypes[".odp"] = "application/vnd.oasis.opendocument.presentation";
    m_mimeTypes[".ods"] = "application/vnd.oasis.opendocument.spreadsheet";
    m_mimeTypes[".odt"] = "application/vnd.oasis.opendocument.text";
    m_mimeTypes[".oga"] = "audio/ogg";
    m_mimeTypes[".ogv"] = "video/ogg";
    m_mimeTypes[".ogx"] = "application/ogg";
    m_mimeTypes[".opus"] = "audio/opus";
    m_mimeTypes[".otf"] = "font/otf";
    m_mimeTypes[".png"] = "image/png";
    m_mimeTypes[".pdf"] = "application/pdf";
    m_mimeTypes[".php"] = "application/x-httpd-php";
    m_mimeTypes[".ppt"] = "application/vnd.ms-powerpoint";
    m_mimeTypes[".pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    m_mimeTypes[".rar"] = "application/vnd.rar";
    m_mimeTypes[".rtf"] = "application/rtf";
    m_mimeTypes[".sh"] = "application/x-sh";
    m_mimeTypes[".svg"] = "image/svg+xml";
    m_mimeTypes[".swf"] = "application/x-shockwave-flash";
    m_mimeTypes[".tar"] = "application/x-tar";
    m_mimeTypes[".tif"] = "image/tiff";
    m_mimeTypes[".ts"] = "video/mp2t";
    m_mimeTypes[".ttf"] = "font/ttf";
    m_mimeTypes[".txt"] = "text/plain";
    m_mimeTypes[".vsd"] = "application/vnd.visio";
    m_mimeTypes[".wav"] = "audio/wav";
    m_mimeTypes[".weba"] = "audio/webm";
    m_mimeTypes[".webm"] = "video/webm";
    m_mimeTypes[".webp"] = "image/webp";
    m_mimeTypes[".woff"] = "font/woff";
    m_mimeTypes[".woff2"] = "font/woff2";
    m_mimeTypes[".xhtml"] = "application/xhtml+xml";
    m_mimeTypes[".xls"] = "application/vnd.ms-excel";
    m_mimeTypes[".xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    m_mimeTypes[".xml"] = "application/xml";
    m_mimeTypes[".xul"] = "application/vnd.mozilla.xul+xml";
    m_mimeTypes[".zip"] = "application/zip";
    m_mimeTypes[".3gp"] = "video/3gpp";
    m_mimeTypes[".3g2"] = "video/3gpp2";
    m_mimeTypes[".7z"] = "application/x-7z-compressed";
}
