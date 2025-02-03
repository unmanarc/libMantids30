#include "htmliengine.h"
#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Protocol_HTTP/api_return.h>
#include "json/value.h"
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <Mantids30/Memory/streamablejson.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <Mantids30/Program_Logs/rpclog.h>

#include <boost/regex.hpp>
#include <fstream>
#include <regex>

using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30::Memory;
using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;
using namespace std;


// TODO: documentar los privilegios cargados de un usuario
Status::eRetCode HTMLIEngine::processResourceFile(APIClientHandler *clientHandler, const std::string &sRealFullPath)
{
    // Drop the MMAP container:
    std::string fileContent;

    if (boost::starts_with(sRealFullPath,"MEM:"))
    {
        // Mem-Static resource.
        fileContent = ((Mantids30::Memory::Containers::B_MEM *)clientHandler->getResponseDataStreamer().get())->toString();
        // the server response will be the default data chunk (reset):
        clientHandler->serverResponse.setDataStreamer(nullptr);
    }
    else
    {
        // the server response will be the default data chunk (reset):
        clientHandler->serverResponse.setDataStreamer(nullptr);
        // Local resource.
        std::ifstream fileStream(sRealFullPath);
        if (!fileStream.is_open())
        {
            clientHandler->log(LEVEL_ERR,"fileServer", 2048, "file not found: %s",sRealFullPath.c_str());
            return HTTP::Status::S_404_NOT_FOUND;
        }
        // Pass the file to a string.
        fileContent = std::string((std::istreambuf_iterator<char>(fileStream)),std::istreambuf_iterator<char>());
        fileStream.close();
    }

    // PRECOMPILE _STATIC_TEXT
    boost::match_flag_type flags = boost::match_default;

    // CINC PROCESSOR:
    //boost::regex exStaticText("<CINC_(?<TAGOPEN>[^>]*)>(?<INCPATH>[^<]+)<\\/CINC_(?<TAGCLOSE>[^>]*)>",boost::regex::icase);
    boost::regex exStaticText("<\\%?include(?<SCRIPT_TAG_NAME>[^\\:]*):[ ]*(?<PATH>[^\\%]+)[ ]*\\%>",boost::regex::icase);

    boost::match_results<string::const_iterator> whatStaticText;
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticText, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag      = string(whatStaticText[0].first, whatStaticText[0].second);
        string tag          = string(whatStaticText[1].first, whatStaticText[1].second);
        string includePath  = string(whatStaticText[2].first, whatStaticText[2].second);
//      string tagClose     = string(whatStaticText[3].first, whatStaticText[3].second);

        // GET THE TAG DATA HERE...
        // The path is relative to documentRootPath (beware: admits transversal)
        std::ifstream fileIncludeStream(clientHandler->config->getDocumentRootPath() + includePath);

        if (fileIncludeStream.is_open())
        {
            std::string includeFileContent((std::istreambuf_iterator<char>(fileIncludeStream)),std::istreambuf_iterator<char>());
            if (!tag.empty() && tag.size()>1 && tag.at(0) == '/')
                boost::replace_all(fileContent,fulltag, "<" + tag.substr(1) + ">" + includeFileContent + "</" + tag.substr(1) + ">" );
            else
                boost::replace_all(fileContent,fulltag, includeFileContent);
        }
        else
        {
            boost::replace_all(fileContent,fulltag, "<!-- HTMLI ENGINE ERROR (FILE NOT FOUND): " + includePath + " -->");

            clientHandler->log(LEVEL_ERR,"fileserver", 2048, "file not found: %s",sRealFullPath.c_str());
        }
    }

    json jVars,jNull;
    jVars["softwareVersion"]   = clientHandler->config->softwareVersion;

    // Fill the jVars with session info (common to every API Server) and extra info (specific to monolith or restful or anything else)
    clientHandler->fillSessionInfo(jVars);
    clientHandler->fillSessionExtraInfo(jVars);

    // %JVAR PROCESSOR:
    boost::regex exStaticJsonInputVar("<\\%?jvar(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<VAR_NAME>[^\\%]+)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonInputVar, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag       = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string varName       = string(whatStaticText[2].first, whatStaticText[2].second);

        bool isSessionVar = clientHandler->doesSessionVariableExist( varName );
        bool isJVar = jVars.isMember(varName);

        // Report as not found.
        if ( !isSessionVar && !isJVar )
        {
            // look in post/get
            clientHandler->log(LEVEL_ERR, "fileserver", 2048, "Main variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
        else
        {
            // jvar is from a set of constructed variables (remote address, user agent, etc...)
            if (isJVar)
            {
                replaceTagByJVar(fileContent,fulltag,jVars[varName],false,scriptVarName);
            }
            // Session vars are set by the application or the JWT token:
            else if (isSessionVar)
            {
                replaceTagByJVar(fileContent,fulltag,clientHandler->getSessionVariableValue(varName),false,scriptVarName);
            }
        }
    }

    // %JSESSVAR PROCESSOR:
    boost::regex exStaticJsonSessionVar("<\\%?jsessvar(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<VAR_NAME>[^\\%]+)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonSessionVar, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag       = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string varName       = string(whatStaticText[2].first, whatStaticText[2].second);

        // Report as not found.
        if ( ! clientHandler->doesSessionVariableExist(varName)   )
        {
            // look in post/get
            clientHandler->log(LEVEL_ERR, "fileserver", 2048, "Main variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
        else
        {
            replaceTagByJVar(fileContent,fulltag,clientHandler->getSessionVariableValue(varName),false,scriptVarName);
        }
    }

    // %JPOSTVAR PROCESSOR:
    boost::regex exStaticJsonPostVar("<\\%?jpostvar(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<VAR_NAME>[^\\%]+)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonPostVar, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag      = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string varName       = string(whatStaticText[2].first, whatStaticText[2].second);

        // Obtain using POST Vars...
        if (clientHandler->clientRequest.getVars(HTTPv1_Base::HTTP_VARS_POST)->exist(varName))
        {
            replaceTagByJVar(fileContent,fulltag,clientHandler->clientRequest.getVars(HTTPv1_Base::HTTP_VARS_POST)->getTValue<std::string>(varName));
        }
        // Report as not found.
        else
        {
            // look in post/get
            clientHandler->log(LEVEL_ERR, "fileserver", 2048, "Post variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
    }

    // %JGETVAR PROCESSOR:
    boost::regex exStaticJsonGetVar("<\\%?jgetvar(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<VAR_NAME>[^\\%]+)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonGetVar, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag      = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string varName       = string(whatStaticText[2].first, whatStaticText[2].second);

        // Obtain using POST Vars...
        if (clientHandler->clientRequest.getVars(HTTPv1_Base::HTTP_VARS_GET)->exist(varName))
        {
           replaceTagByJVar(fileContent,
                             fulltag,
                             clientHandler->clientRequest.getVars(HTTPv1_Base::HTTP_VARS_GET)->getTValue<std::string>(varName)
                             );
        }
        // Report as not found.
        else
        {
            // look in post/get
            clientHandler->log(LEVEL_ERR, "fileserver", 2048, "Get variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
    }

    //%JFUNC PROCESSOR:
    // TODO: como revisar que realmente termine en ) y no haya un ) dentro del json
    boost::regex exStaticJsonFunction("<\\%jfunc(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<FUNCTION_NAME>[^\\(]+)\\((?<FUNCTION_VALUE>[^\\)]*)\\)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonFunction, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag       = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string functionName  = string(whatStaticText[2].first, whatStaticText[2].second);
        string functionInput = string(whatStaticText[3].first, whatStaticText[3].second);

        Helpers::Encoders::replaceHexCodes(functionInput);

        Memory::Streams::StreamableJSON jPayloadOutStr;

        API::APIReturn apiReturn;

        Json::Value vars;
        // Parse the JSON header using the JsonCpp library
        Json::CharReaderBuilder reader;
        std::unique_ptr<Json::CharReader> charReader(reader.newCharReader()); // create a unique_ptr to manage the JsonCpp char reader
        Json::Value header;
        std::string errs;

        if (!charReader->parse(functionInput.c_str(), functionInput.c_str() + functionInput.length(), &vars, &errs))
        {
            clientHandler->log(LEVEL_ERR, "fileserver", 2048,
                               "JSON parsing failed for input: '%s' on resource '%s'. Error: %s",
                               functionInput.c_str(), sRealFullPath.c_str(), errs.c_str());
        }

        // Regular expression to split the components (eg. POST/v1/myFunction)
        std::regex exFunctionNameSplit(R"(^([^/]+)/v([^/]+)/(.+)$)");
        std::smatch matches;
        if (std::regex_match(functionName, matches, exFunctionNameSplit))
        {
            std::string METHOD_MODE = matches[1];
            std::string VERSION = matches[2];
            std::string FUNCTION_NAME = matches[3];
            uint32_t xversion = static_cast<uint32_t>(strtoul( VERSION.c_str(), nullptr, 10 ));


            clientHandler->handleAPIRequest(&apiReturn, "/", xversion, METHOD_MODE, FUNCTION_NAME, {},vars );
        }
        else
        {
            clientHandler->handleAPIRequest(&apiReturn, "/", 1, "POST", functionName, {},vars );
        }

        replaceTagByJVar(fileContent,fulltag,*(jPayloadOutStr.getValue()),true,scriptVarName);
    }


    // Stream the generated content...
    clientHandler->getResponseDataStreamer()->writeString(fileContent);
    return HTTP::Status::S_200_OK;
}


void HTMLIEngine::replaceTagByJVar(std::string &content, const std::string &tag, const json &value, bool replaceFirst, const std::string &varName)
{
    Json::FastWriter writer;
    std::string str = writer.write( value );
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    boost::replace_all(str,"<", "\\<");
    boost::replace_all(str,">", "\\>");

    if (!varName.empty() && varName.size()>1 && varName.at(0) == '/')
    {
        str = "<script>\nconst " + varName.substr(1) + " = " + str + ";\n</script>";
    }

    if (!replaceFirst)
        boost::replace_all(content,tag, str);
    else
        boost::replace_first(content,tag, str);
}
