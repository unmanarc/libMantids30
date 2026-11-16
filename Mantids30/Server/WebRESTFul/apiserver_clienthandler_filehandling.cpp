#include "apiserver_clienthandler.h"
#include "htmliengine.h"
#include "resourcesfilter.h"

#include <Mantids30/DataFormat/JSON/json.h>
#include <Mantids30/Core/Memory/b_mmap.h>
#include <Mantids30/Core/Memory/streamable_string.h>
#include <Mantids30/Protocol/HTTP/rsp_status.h>
#include <Mantids30/Scripting/MantidsLang/mantidslang.h>

#include <memory>
#include <optional>
#include <string>

using namespace Mantids30::Protocol;
using namespace Mantids30::Core::Memory;
using namespace Mantids30::Server::WebRESTFul;
using namespace Mantids30;
using namespace std;

void APIServer_ClientHandler::langProcessViaMantidsLang(const LocalRequestedFileInfo &fileInfo)
{
    auto sourceMem = std::dynamic_pointer_cast<Mantids30::Core::Memory::Containers::B_Base>(serverResponse.content.getStreamableObject());
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

    std::shared_ptr<boost::json::value> jsonContext = std::make_shared<boost::json::value>();

    boost::json::value *ctx = jsonContext.get();
    if (!ctx->is_object())
    {
        *ctx = boost::json::object{};
    }
    boost::json::object &root = ctx->as_object();

    boost::json::object session;
    session["isActive"] = isSessionActive();
    root["session"] = session;

    if (currentSessionInfo.authSession)
    {
        boost::json::object &s = ctx->as_object()["session"].as_object();
        s["user"] = currentSessionInfo.authSession->getUser();
        s["domain"] = currentSessionInfo.authSession->getDomain();
        s["roles"] = Mantids30::DataFormat::JSON::fromSet(getSessionRoles());
        s["scopes"] = Mantids30::DataFormat::JSON::fromSet(getSessionScopes());
        s["isImpersonation"] = currentSessionInfo.isImpersonation;
        s["halfID"] = currentSessionInfo.halfSessionId;

        if (currentSessionInfo.isImpersonation)
        {
            s["impersonator"] = currentSessionInfo.authSession->getImpersonator();
        }

        fillSessionExtraInfo(ctx->as_object()["session"]);
    }

    boost::json::object client;
    client["tlsCN"] = clientRequest.networkClientInfo.tlsCommonName;
    client["ip"] = std::string(clientRequest.networkClientInfo.REMOTE_ADDR);
    client["userAgent"] = clientRequest.userAgent;
    root["client"] = client;

    boost::json::object server;
    server["unixTime"] = time(nullptr);
    root["server"] = server;

    boost::json::object script;
    script["fullpath"] = fileInfo.fullPath;
    script["relativePath"] = fileInfo.relativePath;
    root["script"] = script;

    boost::json::object request;
    request["get"] = clientRequest.getVarsBySource(Mantids30::Protocol::HTTP::Source::GET)->toJSON();
    request["post"] = clientRequest.getVarsBySource(Mantids30::Protocol::HTTP::Source::POST)->toJSON();
    root["request"] = request;

    boost::json::object software;
    software["version"] = config->softwareVersion;
    software["description"] = config->softwareDescription;
    software["name"] = config->softwareName;
    root["software"] = software;

    std::shared_ptr<Mantids30::Scripting::MantidsLang> mantidsTemplateLang = std::make_shared<Mantids30::Scripting::MantidsLang>(
        serverResponse.content.getStreamableObject(),
        jsonContext,
        nullptr,
        [this, sourcePath = fileInfo.fullPath](const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &methodType, const std::string &endpointName, const boost::json::value &postParameters) -> boost::json::value
        {
            Mantids30::Protocol::HTTP::APIReturn result = handleAPIRequestFromFile(sourcePath, "/", apiVersion, methodType, endpointName, postParameters);

            boost::json::value *jsonValue = result.responseJSON();

            return jsonValue ? *jsonValue : boost::json::value{};
        });
    mantidsTemplateLang->setDefaultPath(config->getDocumentRootPath());
    serverResponse.setContentDataStreamer(mantidsTemplateLang);
    serverResponse.content.setTransmissionMode(Mantids30::Protocol::HTTP::Content::TransmissionMode::CHUNKS); // Allow connection reusage.
}


Mantids30::Protocol::HTTP::Status::Code APIServer_ClientHandler::langProcessAcceptedResource(uint16_t statusCode, Mantids30::Server::WebRESTFul::ResourcesFilter::ProcessingMode processingMode, const LocalRequestedFileInfo &fileInfo)
{
    Mantids30::Protocol::HTTP::Status::Code acceptedStatus = static_cast<Mantids30::Protocol::HTTP::Status::Code>(statusCode == 0 ? 200 : statusCode);

    switch (processingMode)
    {
    case Mantids30::Server::WebRESTFul::ResourcesFilter::ProcessingMode::HTMLIENGINE:
    {
        if (serverResponse.contentType == "text/html" || serverResponse.contentType == "application/javascript")
        {
            acceptedStatus = HTMLIEngine::processResourceFile(this, fileInfo.fullPath);
        }

        break;
    }

    case Mantids30::Server::WebRESTFul::ResourcesFilter::ProcessingMode::MANTIDSLANG:
    {
        langProcessViaMantidsLang(fileInfo);
        break;
    }

    case Mantids30::Server::WebRESTFul::ResourcesFilter::ProcessingMode::RAW:
    default:
        break;
    }

    return acceptedStatus;
}

Mantids30::Protocol::HTTP::Status::Code APIServer_ClientHandler::handleRegularFileRequest()
{
    Mantids30::Protocol::HTTP::Status::Code ret = Mantids30::Protocol::HTTP::Status::Code::S_404_NOT_FOUND;
    LocalRequestedFileInfo fileInfo;

    if (config->getDocumentRootPath().empty())
    {
        return Mantids30::Protocol::HTTP::Status::Code::S_404_NOT_FOUND;
    }

    if ((resolveLocalFilePathFromURI2(config->getDocumentRootPath(), config->getOverlappedDirectories(), &fileInfo, ".html")
         || resolveLocalFilePathFromURI2(config->getDocumentRootPath(), config->getOverlappedDirectories(), &fileInfo, "index.html")
         || resolveLocalFilePathFromURI2(config->getDocumentRootPath(), config->getOverlappedDirectories(), &fileInfo, ""))
        && !fileInfo.isDirectory)
    {
        Mantids30::Server::WebRESTFul::ResourcesFilter::FilterEvaluationResult evaluationResult;

        if (config->resourceFilter)
        {
            evaluationResult = config->resourceFilter->evaluateURI(fileInfo.relativePath, getSessionScopes(), getSessionRoles(), isSessionActive(),isAdmin());
        }
        else
        {
            Mantids30::Server::WebRESTFul::ResourcesFilter::Action acceptAction;
            acceptAction.type = Mantids30::Server::WebRESTFul::ResourcesFilter::ActionType::ACCEPT;
            acceptAction.statusCode = 200;

            evaluationResult.actions.push_back(std::move(acceptAction));
        }

        if (config->debugResourceFilter)
        {
            std::cout << "FilterEvaluationResult: " << fileInfo.relativePath << " - " << boost::json::serialize(evaluationResult.toJSON()) << std::endl;
        }

        Mantids30::Server::WebRESTFul::ResourcesFilter::ProcessingMode processingMode = Mantids30::Server::WebRESTFul::ResourcesFilter::ProcessingMode::RAW;

        bool terminalActionExecuted = false;

        for (const Mantids30::Server::WebRESTFul::ResourcesFilter::Action &action : evaluationResult.actions)
        {
            switch (action.type)
            {
            case Mantids30::Server::WebRESTFul::ResourcesFilter::ActionType::REPLACE_HEADERS:
            {
                for (const auto &header : action.httpHeaders)
                {
                    serverResponse.headers.replace(header.first, header.second);
                }

                break;
            }

            case Mantids30::Server::WebRESTFul::ResourcesFilter::ActionType::ADD_HEADERS:
            {
                for (const auto &header : action.httpHeaders)
                {
                    serverResponse.headers.add(header.first, header.second);
                }

                break;
            }

            case Mantids30::Server::WebRESTFul::ResourcesFilter::ActionType::PROCESS_AS:
            {
                processingMode = action.processingMode;
                break;
            }

            case Mantids30::Server::WebRESTFul::ResourcesFilter::ActionType::REDIRECT:
            {
                ret = serverResponse.setRedirectLocation(action.redirectLocation);

                if (action.statusCode != 0)
                {
                    ret = static_cast<Mantids30::Protocol::HTTP::Status::Code>(action.statusCode);
                }

                terminalActionExecuted = true;
                break;
            }

            case Mantids30::Server::WebRESTFul::ResourcesFilter::ActionType::DENY:
            {
                ret = static_cast<Mantids30::Protocol::HTTP::Status::Code>(action.statusCode == 0 ? 403 : action.statusCode);

                terminalActionExecuted = true;
                break;
            }

            case Mantids30::Server::WebRESTFul::ResourcesFilter::ActionType::ACCEPT:
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

    if (ret != Mantids30::Protocol::HTTP::Status::Code::S_200_OK)
    {
        serverResponse.setContentDataStreamer(nullptr);
    }

    if (ret == Mantids30::Protocol::HTTP::Status::Code::S_404_NOT_FOUND && !config->redirectPathOn404.empty())
    {
        ret = serverResponse.setRedirectLocation(config->redirectPathOn404);
    }

    return ret;
}
