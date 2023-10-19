#include "htmliengine.h"
#include <Mantids29/Protocol_HTTP/httpv1_base.h>
#include <Mantids29/Memory/streamablejson.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <Mantids29/Program_Logs/rpclog.h>

#include <fstream>
#include <streambuf>

#include <boost/regex.hpp>

using namespace Mantids29::Program::Logs;
using namespace Mantids29::Network;
using namespace Mantids29::Network::Protocols;
using namespace Mantids29::Network::Protocols::HTTP;
using namespace Mantids29::Memory;
using namespace Mantids29::Network::Servers::Web;
using namespace Mantids29;
using namespace std;

HTMLIEngine::HTMLIEngine()
{

}

// TODO: documentar los privilegios cargados de un usuario
Status::eRetCode HTMLIEngine::processResourceFile(APIClientHandler *clientHandler, const std::string &sRealFullPath)
{
    // Drop the MMAP container:
    std::string fileContent;

    if (boost::starts_with(sRealFullPath,"MEM:"))
    {
        // Mem-Static resource.
        fileContent = ((Mantids29::Memory::Containers::B_MEM *)clientHandler->getResponseDataStreamer().get())->toString();
        // the server response will be the default data chunk (reset):
        clientHandler->m_serverResponse.setDataStreamer(nullptr);
    }
    else
    {
        // the server response will be the default data chunk (reset):
        clientHandler->m_serverResponse.setDataStreamer(nullptr);
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
        // The path is relative to resourcesLocalPath (beware: admits transversal)
        std::ifstream fileIncludeStream(clientHandler->m_config.resourcesLocalPath + includePath);

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
    jVars["softwareVersion"]   = clientHandler->m_config.softwareVersion;
    jVars["userAgent"]         = clientHandler->m_clientRequest.userAgent;
    jVars["userTLSCommonName"] = clientHandler->m_clientRequest.networkClientInfo.tlsCommonName;

    clientHandler->fillUserDataVars(jVars);
    clientHandler->fillSessionVars(jVars);

    // %JVAR PROCESSOR:
    boost::regex exStaticJsonInputVar("<\\%?jvar(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<VAR_NAME>[^\\%]+)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonInputVar, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag       = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string varName       = string(whatStaticText[2].first, whatStaticText[2].second);

        // Report as not found.
        if ( !jVars.isMember(varName) )
        {
            // look in post/get
            clientHandler->log(LEVEL_ERR, "fileserver", 2048, "Main variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
        else
        {
            replaceTagByJVar(fileContent,fulltag,jVars[varName],false,scriptVarName);
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
        if (clientHandler->m_clientRequest.getVars(HTTPv1_Base::HTTP_VARS_POST)->exist(varName))
        {
            replaceTagByJVar(fileContent,fulltag,clientHandler->m_clientRequest.getVars(HTTPv1_Base::HTTP_VARS_POST)->getTValue<std::string>(varName));
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
        if (clientHandler->m_clientRequest.getVars(HTTPv1_Base::HTTP_VARS_GET)->exist(varName))
        {
           replaceTagByJVar(fileContent,
                             fulltag,
                             clientHandler->m_clientRequest.getVars(HTTPv1_Base::HTTP_VARS_GET)->getTValue<std::string>(varName)
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

        replaceHexCodes(functionInput);

        Memory::Streams::StreamableJSON jPayloadOutStr;
        //procJSONWebAPI_Exec(extraAuths,functionName,functionInput, &jPayloadOutStr);
        replaceTagByJVar(fileContent,fulltag,*(jPayloadOutStr.getValue()),true,scriptVarName);
    }


    clientHandler->sessionRenew();

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

void HTMLIEngine::replaceHexCodes(std::string &content)
{
    auto hex2uchar = [](const std::string &t1, const std::string &t2) -> unsigned char {
        auto get16Value = [](unsigned char byte) -> unsigned char {
            if (byte >= 'A' && byte <= 'F') return byte - 'A' + 10;
            else if (byte >= 'a' && byte <= 'f') return byte - 'a' + 10;
            else if (byte >= '0' && byte <= '9') return byte - '0';
            return 0;
        };

        return get16Value(t1.c_str()[0]) * 0x10 + get16Value(t2.c_str()[0]);
    };

    boost::match_results<string::const_iterator> whatStaticText;
    boost::regex exStaticJsonFunction("\\\\0*x(?<V1>[0123456789ABCDEF])(?<V2>[0123456789ABCDEF])",boost::regex::icase);
    boost::match_flag_type flags = boost::match_default;

    for (string::const_iterator start = content.begin(), end =  content.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonFunction, flags); // FIND REGEXP
         start = content.begin(), end =  content.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag       = string(whatStaticText[0].first, whatStaticText[0].second);
        string v1  = string(whatStaticText[1].first, whatStaticText[1].second);
        string v2  = string(whatStaticText[2].first, whatStaticText[2].second);

        std::string replSrc(1,hex2uchar(v1,v2));
        boost::replace_all(content,fulltag, replSrc);
    }
}
