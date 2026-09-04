#include "apiserver_clienthandler.h"
#include "htmliengine.h"
#include "resourcesfilter.h"

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/b_mmap.h>
#include <Mantids30/Memory/streamable_string.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Scripts_MantidsLang/mantidslang.h>

#include <json/value.h>
#include <memory>
#include <optional>
#include <string>

using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocol;
using namespace Mantids30::Memory;
using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;
using namespace std;

void APIServer_ClientHandler::langProcessViaMantidsLang(const LocalRequestedFileInfo &fileInfo)
{
    auto sourceMem = std::dynamic_pointer_cast<Memory::Containers::B_Base>(serverResponse.content.getStreamableObject());
    // Heuristic for detecting
    if (sourceMem)
    {
        if (sourceMem->find("{{",2)!=std::nullopt && sourceMem->find("}}",2)!=std::nullopt)
        {
            // Variable script, send everytime.
            // Default to a conservative cache policy for dynamic responses.
            serverResponse.cacheControl.optionNoCache = true;
            serverResponse.cacheControl.optionNoStore = true;
            serverResponse.cacheControl.optionMustRevalidate = true;
            serverResponse.headers.remove("Last-Modified");
            serverResponse.etag.clear();
        }
        else
        {
            // Static Script, always the same (rely on date/inode/etc etag).
            serverResponse.cacheControl.optionNoCache = true;
            serverResponse.cacheControl.optionNoStore = false;
            serverResponse.cacheControl.optionMustRevalidate = true;
        }
    }
    else
    {
        // No object?
        return;
    }

    std::shared_ptr<Json::Value> jsonContext = std::make_shared<Json::Value>();

    (*jsonContext)["session"]["isActive"] = isSessionActive();

    if (currentSessionInfo.authSession)
    {
        (*jsonContext)["session"]["user"] = currentSessionInfo.authSession->getUser();
        (*jsonContext)["session"]["domain"] = currentSessionInfo.authSession->getDomain();
        (*jsonContext)["session"]["roles"] = Helpers::JSON::fromSet(getSessionRoles());
        (*jsonContext)["session"]["scopes"] = Helpers::JSON::fromSet(getSessionScopes());
        (*jsonContext)["session"]["isImpersonation"] = currentSessionInfo.isImpersonation;
        (*jsonContext)["session"]["halfID"] = currentSessionInfo.halfSessionId;

        if (currentSessionInfo.isImpersonation)
        {
            (*jsonContext)["session"]["impersonator"] = currentSessionInfo.authSession->getImpersonator();
        }

        fillSessionExtraInfo((*jsonContext)["session"]);
    }

    (*jsonContext)["client"]["tlsCN"] = clientRequest.networkClientInfo.tlsCommonName;
    (*jsonContext)["client"]["ip"] = clientRequest.networkClientInfo.REMOTE_ADDR;
    (*jsonContext)["client"]["userAgent"] = clientRequest.userAgent;

    (*jsonContext)["server"]["unixTime"] = time(nullptr);

    (*jsonContext)["script"]["fullpath"] = fileInfo.fullPath;
    (*jsonContext)["script"]["relativePath"] = fileInfo.relativePath;

    (*jsonContext)["request"]["get"] = clientRequest.getVarsBySource(HTTP::Source::GET)->toJSON();
    (*jsonContext)["request"]["post"] = clientRequest.getVarsBySource(HTTP::Source::POST)->toJSON();

    (*jsonContext)["software"]["version"] = config->softwareVersion;
    (*jsonContext)["software"]["description"] = config->softwareDescription;
    (*jsonContext)["software"]["name"] = config->softwareName;

    std::shared_ptr<Scripts::MantidsLang> mantidsTemplateLang = std::make_shared<Scripts::MantidsLang>(
        serverResponse.content.getStreamableObject(),
        jsonContext,
        nullptr,
        [this](const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &methodType, const std::string &endpointName, const Json::Value &postParameters) -> Json::Value
        {
            API::APIReturn result = handleAPIRequest("/", apiVersion, methodType, endpointName, postParameters);

            Json::Value *jsonValue = result.responseJSON();

            return jsonValue ? *jsonValue : Json::nullValue;
        });
    mantidsTemplateLang->setDefaultPath(config->getDocumentRootPath());
    serverResponse.setContentDataStreamer(mantidsTemplateLang);
    serverResponse.content.setTransmissionMode(Protocol::HTTP::Content::TransmissionMode::CHUNKS); // Allow connection reusage.
}


HTTP::Status::Code APIServer_ClientHandler::langProcessAcceptedResource(uint16_t statusCode, API::Web::ResourcesFilter::ProcessingMode processingMode, const LocalRequestedFileInfo &fileInfo)
{
    HTTP::Status::Code acceptedStatus = static_cast<HTTP::Status::Code>(statusCode == 0 ? 200 : statusCode);

    switch (processingMode)
    {
    case API::Web::ResourcesFilter::ProcessingMode::HTMLIENGINE:
    {
        if (serverResponse.contentType == "text/html" || serverResponse.contentType == "application/javascript")
        {
            acceptedStatus = HTMLIEngine::processResourceFile(this, fileInfo.fullPath);
        }

        break;
    }

    case API::Web::ResourcesFilter::ProcessingMode::MANTIDSLANG:
    {
        langProcessViaMantidsLang(fileInfo);
        break;
    }

    case API::Web::ResourcesFilter::ProcessingMode::RAW:
    default:
        break;
    }

    return acceptedStatus;
}

HTTP::Status::Code APIServer_ClientHandler::handleRegularFileRequest()
{
    HTTP::Status::Code ret = HTTP::Status::Code::S_404_NOT_FOUND;
    LocalRequestedFileInfo fileInfo;

    if (config->getDocumentRootPath().empty())
    {
        return HTTP::Status::Code::S_404_NOT_FOUND;
    }

    if ((resolveLocalFilePathFromURI2(config->getDocumentRootPath(), config->getOverlappedDirectories(), &fileInfo, ".html")
         || resolveLocalFilePathFromURI2(config->getDocumentRootPath(), config->getOverlappedDirectories(), &fileInfo, "index.html")
         || resolveLocalFilePathFromURI2(config->getDocumentRootPath(), config->getOverlappedDirectories(), &fileInfo, ""))
        && !fileInfo.isDirectory)
    {
        API::Web::ResourcesFilter::FilterEvaluationResult evaluationResult;

        if (config->resourceFilter)
        {
            evaluationResult = config->resourceFilter->evaluateURI(fileInfo.relativePath, getSessionScopes(), getSessionRoles(), isSessionActive(),isAdmin());
        }
        else
        {
            API::Web::ResourcesFilter::Action acceptAction;
            acceptAction.type = API::Web::ResourcesFilter::ActionType::ACCEPT;
            acceptAction.statusCode = 200;

            evaluationResult.actions.push_back(std::move(acceptAction));
        }

        if (config->debugResourceFilter)
        {
            std::cout << "FilterEvaluationResult: " << fileInfo.relativePath << " - " << evaluationResult.toJSON().toStyledString() << std::endl;
        }

        API::Web::ResourcesFilter::ProcessingMode processingMode = API::Web::ResourcesFilter::ProcessingMode::RAW;

        bool terminalActionExecuted = false;

        for (const API::Web::ResourcesFilter::Action &action : evaluationResult.actions)
        {
            switch (action.type)
            {
            case API::Web::ResourcesFilter::ActionType::REPLACE_HEADERS:
            {
                for (const auto &header : action.httpHeaders)
                {
                    serverResponse.headers.replace(header.first, header.second);
                }

                break;
            }

            case API::Web::ResourcesFilter::ActionType::ADD_HEADERS:
            {
                for (const auto &header : action.httpHeaders)
                {
                    serverResponse.headers.add(header.first, header.second);
                }

                break;
            }

            case API::Web::ResourcesFilter::ActionType::PROCESS_AS:
            {
                processingMode = action.processingMode;
                break;
            }

            case API::Web::ResourcesFilter::ActionType::REDIRECT:
            {
                ret = serverResponse.setRedirectLocation(action.redirectLocation);

                if (action.statusCode != 0)
                {
                    ret = static_cast<HTTP::Status::Code>(action.statusCode);
                }

                terminalActionExecuted = true;
                break;
            }

            case API::Web::ResourcesFilter::ActionType::DENY:
            {
                ret = static_cast<HTTP::Status::Code>(action.statusCode == 0 ? 403 : action.statusCode);

                terminalActionExecuted = true;
                break;
            }

            case API::Web::ResourcesFilter::ActionType::ACCEPT:
            {
                ret = langProcessAcceptedResource(action.statusCode, processingMode, fileInfo);

                terminalActionExecuted = true;
                break;
            }
            }

            if (terminalActionExecuted)
            {
                break;
            }
        }

        // Default: accept.
        if (!terminalActionExecuted)
        {
            ret = langProcessAcceptedResource(200, processingMode, fileInfo);
        }
    }

    if (ret != HTTP::Status::Code::S_200_OK)
    {
        serverResponse.setContentDataStreamer(nullptr);
    }

    if (ret == HTTP::Status::Code::S_404_NOT_FOUND && !config->redirectPathOn404.empty())
    {
        ret = serverResponse.setRedirectLocation(config->redirectPathOn404);
    }

    return ret;
}