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
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Memory;
using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;
using namespace std;

// TODO: documentar los privilegios cargados de un usuario
// TODO: create a TTL for start =  fileContent.begin(); and end = fileContent.end(); on loops to avoid infinite loops in cross-references...

string HTMLIEngine::replaceByJVar(
    const json &value, const std::string &scriptVarName)
{
    Json::FastWriter writer;
    std::string str = writer.write(value);
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

    boost::replace_all(str, "<", "\\<");
    boost::replace_all(str, ">", "\\>");

    if (!scriptVarName.empty())
    {
        str = "<script>\nconst " + scriptVarName + " = " + str + ";\n</script>";
    }

    return str;
}

HTTP::Status::Codes HTMLIEngine::processResourceFile(APIClientHandler *clientHandler, const std::string &sRealFullPath)
{
    // Drop the MMAP container:
    std::string fileContent;

    if (boost::starts_with(sRealFullPath, "MEM:"))
    {
        // Mem-Static resource.
        fileContent = ((Mantids30::Memory::Containers::B_MEM *)clientHandler->getResponseDataStreamer().get())->toString();
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

    // CINC PROCESSOR:
    procResource_HTMLIEngineInclude(sRealFullPath, fileContent, clientHandler);
    procResource_JProcessor(sRealFullPath, fileContent, clientHandler);

    // Stream the generated content...
    clientHandler->getResponseDataStreamer()->writeString(fileContent);
    return HTTP::Status::S_200_OK;
}

void HTMLIEngine::procResource_JProcessor(
    const std::string &sRealFullPath, std::string &input, APIClientHandler *clientHandler)
{
    std::regex re("<%[jJ]([a-zA-Z\\/]+):[ ]*([^%]*)[ ]*%>");
    size_t pos = 0;

    // Search every J processor in one big loop and replace in place and continue... so won't be a chance to re-ingest anything...
    while (true)
    {
        std::smatch match;
        std::string::const_iterator start_it = input.begin() + pos;
        std::string::const_iterator end_it = input.end();

        if (!std::regex_search(start_it, end_it, match, re))
        {
            break;
        }

        int absolute_pos = static_cast<int>(std::distance(input.cbegin(), match[0].first));
        int length = match.length();

        if ( match.size() >=3 )
        {
            std::string command = match[1];
            std::string value = match[2];
            std::string replacedBy = "null";
            std::string scriptVarName = "varName";

            if ( boost::istarts_with( command, "VAR/" ) )
            {
                scriptVarName=command.c_str()+3+1;
                // %JVAR PROCESSOR:
                replacedBy = procResource_HTMLIEngineJVAR(scriptVarName,value, sRealFullPath,clientHandler);
            }
            if ( boost::istarts_with( command, "GETVAR/" ) )
            {
                scriptVarName=command.c_str()+6+1;
                // %JGETVAR PROCESSOR:
                replacedBy = procResource_HTMLIEngineJGETVAR(scriptVarName,value, sRealFullPath,clientHandler);
            }
            if ( boost::istarts_with( command, "POSTVAR/" ) )
            {
                scriptVarName=command.c_str()+7+1;
                // %JPOSTVAR PROCESSOR:
                replacedBy = procResource_HTMLIEngineJPOSTVAR(scriptVarName,value, sRealFullPath,clientHandler);
            }
            if ( boost::istarts_with( command, "FUNC/" ) )
            {
                scriptVarName=command.c_str()+4+1;
                // %JFUNC PROCESSOR:
                replacedBy = procResource_HTMLIEngineJFUNC(sRealFullPath,scriptVarName, value, clientHandler);
            }
            if ( boost::istarts_with( command, "SESS/" ) )
            {
                scriptVarName=command.c_str()+4+1;
                // %JSESSVAR PROCESSOR:
                replacedBy = procResource_HTMLIEngineJSESSVAR(scriptVarName,value, sRealFullPath,clientHandler);
            }

            input.replace(absolute_pos, length, replacedBy);
            pos = absolute_pos + replacedBy.size();
        }
    }
}

std::string HTMLIEngine::procResource_HTMLIEngineJSESSVAR(
    const std::string &scriptVarName, const std::string &varName, const std::string &sRealFullPath, APIClientHandler *clientHandler)
{
    // %JSESSVAR PROCESSOR:

    // Report as not found.
    if ( ! clientHandler->doesSessionVariableExist(varName)   )
    {
        // look in post/get
        clientHandler->log(LEVEL_ERR, "fileserver", 2048, "Main variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
        return replaceByJVar(Json::Value::null, scriptVarName);
    }
    else
    {
        return replaceByJVar(clientHandler->getSessionVariableValue(varName),scriptVarName);
    }
}

std::string HTMLIEngine::procResource_HTMLIEngineJVAR(const std::string &scriptVarName, const std::string &varName,const std::string &sRealFullPath, APIClientHandler *clientHandler)
{
    json jVars,jNull;
    jVars["softwareVersion"]   = clientHandler->config->softwareVersion;

    // Fill the jVars with session info (common to every API Server) and extra info (specific to monolith or restful or anything else)
    clientHandler->fillSessionInfo(jVars);
    clientHandler->fillSessionExtraInfo(jVars);


    bool isSessionVar = clientHandler->doesSessionVariableExist( varName );
    bool isJVar = jVars.isMember(varName);

    // Report as not found.
    if ( !isSessionVar && !isJVar )
    {
        // look in post/get
        clientHandler->log(LEVEL_ERR, "fileserver", 2048, "Main variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
        return replaceByJVar(Json::Value::null, scriptVarName);
    }
    else
    {
        // jvar is from a set of constructed variables (remote address, user agent, etc...)
        if (isJVar)
        {
            return replaceByJVar(jVars[varName], scriptVarName);
        }
        // Session vars are set by the application or the JWT token:
        else if (isSessionVar)
        {
            return replaceByJVar(clientHandler->getSessionVariableValue(varName), scriptVarName);
        }

        return replaceByJVar(Json::Value::null, scriptVarName);
    }
}

// Function to process JGETVAR tags in the file content
std::string HTMLIEngine::procResource_HTMLIEngineJGETVAR(const std::string &scriptVarName, const std::string &varName,const std::string &sRealFullPath, APIClientHandler *clientHandler)
{
    // Obtain using GET Vars...
    if (clientHandler->clientRequest.getVars(HTTP::VARS_GET)->exist(varName))
    {
        return replaceByJVar(clientHandler->clientRequest.getVars(HTTP::VARS_GET)->getTValue<std::string>(varName), scriptVarName);
    }
    // Report as not found.
    else
    {
        // look in post/get
        clientHandler->log(LEVEL_ERR, "fileserver", 2048, "GET variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
        return replaceByJVar(Json::Value::null, scriptVarName);
    }
}


std::string HTMLIEngine::procResource_HTMLIEngineJPOSTVAR(
    const std::string &scriptVarName, const std::string &varName, const std::string &sRealFullPath, APIClientHandler *clientHandler)
{
    // Obtain using POST Vars...
    if (clientHandler->clientRequest.getVars(HTTP::VARS_POST)->exist(varName))
    {
        return replaceByJVar(clientHandler->clientRequest.getVars(HTTP::VARS_POST)->getTValue<std::string>(varName),scriptVarName);
    }
    // Report as not found.
    else
    {
        // look in post/get
        clientHandler->log(LEVEL_ERR, "fileserver", 2048, "POST variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
        return replaceByJVar(Json::Value::null, scriptVarName);
    }
}

json HTMLIEngine::procJAPI_Exec(
    const std::string &sRealFullPath, APIClientHandler *clientHandler, const std::string &functionName, const std::string &functionInput)
{

    API::APIReturn apiReturn;

    Json::Value vars;
    // Parse the JSON header using the JsonCpp library
    Json::CharReaderBuilder reader;
    std::unique_ptr<Json::CharReader> charReader(reader.newCharReader()); // create a unique_ptr to manage the JsonCpp char reader
    Json::Value header;
    std::string errs;

    if (!charReader->parse(functionInput.c_str(), functionInput.c_str() + functionInput.length(), &vars, &errs))
    {
        clientHandler->log(LEVEL_ERR, "fileserver", 4096,
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


    return apiReturn.toJSON();
}


std::string HTMLIEngine::procResource_HTMLIEngineJFUNC(const std::string & sRealFullPath,
    const std::string &scriptVarName, const std::string &functionDef, APIClientHandler *clientHandler)
{
    // TODO: como revisar que realmente termine en ) y no haya un ) dentro del json
    std::regex exStaticJsonFunction("([^\\(]+)\\(([^\\)]*)\\)");

    std::smatch whatStaticText;
    std::string::const_iterator start = functionDef.begin();
    std::string::const_iterator end = functionDef.end();

    // Search for matches in the file content
    if  (std::regex_search(start, end, whatStaticText, exStaticJsonFunction))
    {
        // The full tag found in the file content (e.g., <%jfuncVar: Function(param)%>)
        std::string fulltag = whatStaticText[0].str();

        // Second group: function name (e.g., "Function")
        std::string functionName = whatStaticText[1].str();

        // Third group: function input/parameters (e.g., "param")
        std::string functionInput = whatStaticText[2].str();
        Helpers::Encoders::replaceHexCodes(functionInput);

        return replaceByJVar(procJAPI_Exec(sRealFullPath,clientHandler, functionName, functionInput), scriptVarName);
    }

    return replaceByJVar(Json::Value::null, scriptVarName);
}


// Function to process the HTMLI include tags within the file content
void HTMLIEngine::procResource_HTMLIEngineInclude(
    const std::string &sRealFullPath, std::string &fileContent, APIClientHandler *clientHandler)
{

    // PRECOMPILE _STATIC_TEXT
    boost::match_flag_type flags = boost::match_default;

    // CINC PROCESSOR:
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

        // // Move the start iterator to the beginning (maybe we need to reprocess the whole thing after some modifications)...
        // start =  fileContent.begin();
        // end = fileContent.end();
    }
}




