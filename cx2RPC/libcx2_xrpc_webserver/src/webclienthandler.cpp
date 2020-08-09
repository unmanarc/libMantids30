#include "webclienthandler.h"

#include <cx2_mem_vars/b_mmap.h>

using namespace CX2::Network::HTTP;
using namespace CX2::Memory;
using namespace CX2::RPC::Web;
using namespace CX2::RPC;
using namespace CX2;
using namespace std;

WebClientHandler::WebClientHandler(void *parent, Memory::Streams::Streamable *sock) : HTTPv1_Server(sock)
{
    // TODO: logs?
    useFormattedJSONOutput = true;
}

WebClientHandler::~WebClientHandler()
{
}

void WebClientHandler::setAuthenticators(Authorization::IAuth_Domains *authenticator)
{
    authDomains = authenticator;
}

eHTTP_RetCode WebClientHandler::processClientRequest()
{
    eHTTP_RetCode ret  = HTTP_RET_404_NOT_FOUND;
    if (getRequestURI() == "/api") return processRPCRequest();

    std::string reqFullPath = resourcesLocalPath + getRequestURI();

    char * cFullPath;
    // Detect transversal:
    if ((cFullPath=realpath(reqFullPath.c_str(), nullptr))!=nullptr)
    {
        if (strlen(cFullPath)<resourcesLocalPath.size() || memcmp(resourcesLocalPath.c_str(),cFullPath,resourcesLocalPath.size())!=0)
        {
            // Transversal directory attempt TODO: report.
        }
        else
        {
            // No transversal detected.
            std::string sRealURI = cFullPath+resourcesLocalPath.size();

            sFilterEvaluation e;
            e.accept = true;
            if (resourceFilter)
            {
                // Evaluate...
                std::string sSessionId = getRequestCookie("sessionId");
                uint64_t uMaxAge;

                Authorization::Session::IAuth_Session * hSession = sessionsManager->openSession(sSessionId, &uMaxAge);
                CX2::Authorization::IAuth * authorizer = hSession?authDomains->openDomain(hSession->getAuthDomain()) : nullptr;

                e = resourceFilter->evaluateAction(sRealURI,hSession, authorizer);

                if (hSession)
                    sessionsManager->closeSession(sSessionId);

            }
            if (e.accept)
            {
                if (e.location.empty())
                {
                    Containers::B_MMAP * bFile = new Containers::B_MMAP;
                    if (bFile->referenceFile(cFullPath,true,false))
                    {
                        // File Found / Readable.
                        setResponseDataStreamer(bFile,true);
                        setResponseContentType("",false);
                        ret = HTTP_RET_200_OK;
                    }
                    else
                        delete bFile;
                }
                else
                {
                    ret = setResponseRedirect( e.location );
                }
            }
        }
    }
    if (cFullPath) free(cFullPath);

    return ret;
}

eHTTP_RetCode WebClientHandler::processRPCRequest()
{
    bool bDestroySession = false, bCloseSessionHandler = false, bDeleteSession = false;
    Json::Reader reader;
    std::string sSessionId, sMethodName, sRPCMode;
    eHTTP_RetCode eHTTPResponseCode = HTTP_RET_404_NOT_FOUND;
    Json::Value jPayloadIn;
    Request request;

    // COOKIES...
    sSessionId = getRequestCookie("sessionId");

    // GET VARS
    sRPCMode = getRequestVars(HTTP_VARS_GET)->getStringValue("mode");
    sMethodName = getRequestVars(HTTP_VARS_GET)->getStringValue("method");

    // POST VARS
    if (!request.setAuthentications(getRequestVars(HTTP_VARS_POST)->getStringValue("auths")))
        return HTTP_RET_400_BAD_REQUEST;
    if (!getRequestVars(HTTP_VARS_POST)->getStringValue("payload").empty() && !reader.parse(getRequestVars(HTTP_VARS_POST)->getStringValue("payload"), jPayloadIn))
        return HTTP_RET_400_BAD_REQUEST;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    // OPEN THE SESSION HERE:
    uint64_t uMaxAge;
    Authorization::Session::IAuth_Session * hSession = sessionsManager->openSession(sSessionId, &uMaxAge);
    if (hSession)
        bCloseSessionHandler = true;
    else
        sSessionId = ""; // INVALID SESSION ID.

    // Not session found and persistent session has been requested.
    if (!hSession && sRPCMode == "AUTH")
    {
        // Authenticate...
        for (const uint32_t & passIdx : request.getAuthenticationsIdxs())
        {
            Authorization::DataStructs::AuthReason authReason;
            sSessionId = persistentAuthentication( getRequestVars(HTTP_VARS_POST)->getStringValue("user"),
                                                   getRequestVars(HTTP_VARS_POST)->getStringValue("domain"),
                                                   request.getAuthentication(passIdx),
                                                   hSession, &authReason);
            (*(jPayloadOutStr->getValue()))["auth"][std::to_string(passIdx)]["reasonTxt"] = getAuthReasonText(authReason);
            (*(jPayloadOutStr->getValue()))["auth"][std::to_string(passIdx)]["reasonVal"] = static_cast<Json::UInt>(authReason);

            if (!hSession && !sSessionId.empty())
            {
                hSession = sessionsManager->openSession(sSessionId,&uMaxAge);
                if (hSession)
                    bCloseSessionHandler = true;
            }
        }

        if (hSession)
        {
            setResponseSecureCookie("sessionId", sSessionId, uMaxAge);
            eHTTPResponseCode = HTTP_RET_200_OK;
        }
        else
        {
            eHTTPResponseCode = HTTP_RET_401_UNAUTHORIZED;
        }
    }

    if ( sRPCMode == "EXEC" )
    {
        // Open a temporary session if there is no persistent session opened
        if (!hSession)
        {
            ////////////////////////////////////
            // Create Temporary Session
            ////////////////////////////////////
            hSession = new Authorization::Session::IAuth_Session;

            hSession->setAuthUser(getRequestVars(HTTP_VARS_POST)->getStringValue("user"));
            hSession->setAuthDomain(getRequestVars(HTTP_VARS_POST)->getStringValue("domain"));
            bDeleteSession = true;
        }

        std::set<uint32_t> extraTmpIndexes;
        for (const uint32_t & passIdx : request.getAuthenticationsIdxs())
        {
            Authorization::DataStructs::AuthReason authReason;

            if ((authReason=temporaryAuthentication(request.getAuthentication(passIdx),hSession)) == Authorization::DataStructs::AUTH_REASON_AUTHENTICATED)
            {
                extraTmpIndexes.insert(passIdx);

                if (bDeleteSession)
                {
                    // No persistent session, so make this AUTH persistent for this temporary session...
                    hSession->registerPersistentAuthentication(passIdx,Authorization::DataStructs::AUTH_REASON_AUTHENTICATED);
                }
            }
            else
            {
                /*
                 * // TODO:
                (*outJStreamable.getValue())["auth"][std::to_string(passIdx)]["reasonTxt"] = getAuthReasonText(authReason);
                (*outJStreamable.getValue())["auth"][std::to_string(passIdx)]["reasonVal"] = static_cast<Json::UInt>(authReason);
                */
            }
        }

        auto authorizer = authDomains->openDomain(hSession->getAuthDomain());
        if (authorizer)
        {
            Json::Value reasons;
            auto i = methodsManager->validateRPCMethodPerms( authorizer,  hSession, sMethodName, extraTmpIndexes, &reasons);
            authDomains->closeDomain(hSession->getAuthDomain());

            switch (i)
            {
            case VALIDATION_OK:
            {
                // TODO: retcode..
                hSession->updateLastActivity();
                methodsManager->runRPCMethod(authDomains,hSession->getAuthDomain(), hSession, sMethodName, jPayloadIn, jPayloadOutStr->getValue());
                eHTTPResponseCode = HTTP_RET_200_OK;
            }break;
            case VALIDATION_NOTAUTHORIZED:
            {
                // not enough permissions.
                (*(jPayloadOutStr->getValue()))["auth"]["reasons"] = reasons;
                eHTTPResponseCode = HTTP_RET_401_UNAUTHORIZED;
            }break;
            case VALIDATION_METHODNOTFOUND:
            default:
            {
                // not enough permissions.
                eHTTPResponseCode = HTTP_RET_404_NOT_FOUND;
            }break;
            }
        }
        else
        {
            eHTTPResponseCode = HTTP_RET_404_NOT_FOUND;
        }

        if (bDeleteSession)
        {
            ////////////////////////////////////
            // Temporary session mgmt
            ////////////////////////////////////
            delete hSession;
            hSession = nullptr;
        }

    }

    // QUIT: Should be authenticated in a persistent fashion...
    if ( hSession && sRPCMode == "QUIT" )
    {
        eHTTPResponseCode = HTTP_RET_200_OK;
        bDestroySession = true;
    }

    // Close the openned session.
    if (bCloseSessionHandler)
    {
        setResponseSecureCookie("sessionId", sSessionId, uMaxAge);
        sessionsManager->closeSession(sSessionId);
    }
    if (bDestroySession)
        sessionsManager->destroySession(sSessionId);

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);

    // return the HTTP response code.
    return eHTTPResponseCode;
}

void WebClientHandler::setSessionsManagger(SessionsManager *value)
{
    sessionsManager = value;
}

void WebClientHandler::setRemoteIP(const std::string &value)
{
    remoteIP = value;
}

std::string WebClientHandler::persistentAuthentication(const string &userName, const string &domainName, const Authentication &authData, Authorization::Session::IAuth_Session *session, Authorization::DataStructs::AuthReason * authReason)
{
    std::string sessionId;
    if (!session && authData.getPassIndex()!=0) return sessionId;

    *authReason = Authorization::DataStructs::AUTH_REASON_INVALID_DOMAIN;
    Json::Value payload;

    auto auth = authDomains->openDomain(domainName);
    if (auth)
    {
        *authReason = auth->authenticate(userName,authData.getUserPass(),authData.getPassIndex());
        authDomains->closeDomain(domainName);
    }

    if (!session && *authReason == Authorization::DataStructs::AUTH_REASON_AUTHENTICATED)
    {
        session = new Authorization::Session::IAuth_Session;
        session->registerPersistentAuthentication(userName,domainName,authData.getPassIndex(),*authReason);
        sessionId = sessionsManager->addSession(session);
    }

    return sessionId;
}

Authorization::DataStructs::AuthReason WebClientHandler::temporaryAuthentication(const Authentication &authData, Authorization::Session::IAuth_Session *session)
{
    Authorization::DataStructs::AuthReason eReason;
    if (!session) return Authorization::DataStructs::AUTH_REASON_UNAUTHENTICATED;

    std::string userName = session->getAuthUser();
    std::string domainName = session->getAuthDomain();

    auto auth = authDomains->openDomain(domainName);
    if (!auth)
        eReason = Authorization::DataStructs::AUTH_REASON_INVALID_DOMAIN;
    else
    {
        eReason = auth->authenticate( userName,authData.getUserPass(),authData.getPassIndex()); // Authenticate in a non-persistent fashion.
        authDomains->closeDomain(domainName);
    }

    return eReason;
}

void WebClientHandler::setResourcesLocalPath(const std::string &value)
{
    resourcesLocalPath = value;
}

void WebClientHandler::setResourceFilter(ResourcesFilter *value)
{
    resourceFilter = value;
}



void WebClientHandler::setUseFormattedJSONOutput(bool value)
{
    useFormattedJSONOutput = value;
}

void WebClientHandler::setMethodsManager(MethodsManager *value)
{
    methodsManager = value;
}
