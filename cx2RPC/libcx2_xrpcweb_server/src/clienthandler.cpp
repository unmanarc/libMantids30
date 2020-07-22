#include "clienthandler.h"
#include <cx2_xrpc_common/retcodes.h>
#include <cx2_xrpc_common/request.h>

using namespace CX2::RPC::XRPCWeb;
using namespace CX2;
using namespace std;

ClientHandler::ClientHandler(void *parent, Memory::Streams::Streamable *sock) : Network::Parsers::HTTPv1_Server(sock)
{
    // TODO: logs?
    useFormattedJSONOutput = true;
}

ClientHandler::~ClientHandler()
{
}

void ClientHandler::setAuthenticators(Authorization::IAuth_Domains *authenticator)
{
    authDomains = authenticator;
}

Network::Parsers::HttpRetCode ClientHandler::processclientRequest()
{
    bool destroySession = false, closeSession = false, deleteSession = false;
    std::string sessionId;
    Network::Parsers::HttpRetCode httpResponseCode = Network::Parsers::HTTP_RET_404_NOT_FOUND;
    // TODO: max post size?
    Json::Value extraInfoOut, payloadOut;
    XRPC::Request request, response;
    Memory::Streams::Streamable * out = getFullResponse().contentData->getStreamableOuput();

    response.setRetCode(XRPC::METHOD_RET_CODE_METHODNOTFOUND);

    // GET VARS
    request.setRpcMode(getVars(Network::Parsers::HTTP_VARS_GET)->getStringValue("mode"));
    request.setReqId( strtoull ( getVars(Network::Parsers::HTTP_VARS_GET)->getStringValue("reqId").c_str(),nullptr,10 ) );
    request.setRetCode( strtol( getVars(Network::Parsers::HTTP_VARS_GET)->getStringValue("retCode").c_str(),nullptr,10 ) );
    request.setMethodName(getVars(Network::Parsers::HTTP_VARS_GET)->getStringValue("method") );

    // POST VARS
    request.setSessionID(getVars(Network::Parsers::HTTP_VARS_POST)->getStringValue("sessionId"));
    if (!request.setAuthentications(getVars(Network::Parsers::HTTP_VARS_POST)->getStringValue("auths")))
        return Network::Parsers::HTTP_RET_400_BAD_REQUEST;
    if (!request.setPayload(getVars(Network::Parsers::HTTP_VARS_POST)->getStringValue("payload")))
        return Network::Parsers::HTTP_RET_400_BAD_REQUEST;
    if (!request.setExtraInfo(getVars(Network::Parsers::HTTP_VARS_POST)->getStringValue("extraInfo")))
        return Network::Parsers::HTTP_RET_400_BAD_REQUEST;

    // RESPONSE DEFAULTS
    response.setRpcMode(request.getRpcMode());
    response.setMethodName(request.getMethodName());
    response.setRpcMode(request.getRpcMode());
    response.setReqId(request.getReqId());


    // OPEN THE SESSION HERE:
    Authorization::Session::IAuth_Session * session = sessionsManagger->openSession(request.getSessionID());
    if (session)
    {
        sessionId = session->getSessionId();
        response.setSessionID(session->getSessionId());
        closeSession = true;
    }

    // Not session found and persistent session has been requested.
    if (!session && request.getRpcMode() == "AUTH")
    {
        // Authenticate...
        for (const uint32_t & passIdx : request.getAuthenticationsIdxs())
        {
            Authorization::DataStructs::AuthReason authReason;
            session = persistentAuthentication( getVars(Network::Parsers::HTTP_VARS_POST)->getStringValue("user"),
                                                getVars(Network::Parsers::HTTP_VARS_POST)->getStringValue("domain"),
                                                request.getAuthentication(passIdx),
                                                session, &authReason);
            extraInfoOut["auth"][std::to_string(passIdx)]["reasonTxt"] = getAuthReasonText(authReason);
            extraInfoOut["auth"][std::to_string(passIdx)]["reasonVal"] = static_cast<Json::UInt>(authReason);
        }

        if (session)
        {
            sessionId = session->getSessionId();
            response.setSessionID(session->getSessionId());
            response.setRetCode(XRPC::METHOD_RET_CODE_SUCCESS);
            httpResponseCode = Network::Parsers::HTTP_RET_200_OK;
            sessionsManagger->closeSession(request.getSessionID());
        }
        else
        {
            httpResponseCode = Network::Parsers::HTTP_RET_401_UNAUTHORIZED;
            response.setRetCode(XRPC::METHOD_RET_CODE_INVALIDAUTH);
        }
    }

    if ( request.getRpcMode() == "EXEC" )
    {
        // Open a temporary session if there is no persistent session opened
        if (!session)
        {
            ////////////////////////////////////
            // Create Temporary Session
            ////////////////////////////////////
            session = new Authorization::Session::IAuth_Session;
            session->setAuthUser(getVars(Network::Parsers::HTTP_VARS_POST)->getStringValue("user"));
            session->setAuthDomain(getVars(Network::Parsers::HTTP_VARS_POST)->getStringValue("domain"));
            deleteSession = true;
        }

        std::set<uint32_t> extraTmpIndexes;
        for (const uint32_t & passIdx : request.getAuthenticationsIdxs())
        {
            Authorization::DataStructs::AuthReason authReason;

            if ((authReason=temporaryAuthentication(request.getAuthentication(passIdx),session)) == Authorization::DataStructs::AUTH_REASON_AUTHENTICATED)
            {
                extraTmpIndexes.insert(passIdx);

                if (deleteSession)
                {
                    // No persistent session, so make this AUTH persistent for this temporary session...
                    session->registerPersistentAuthentication(passIdx,Authorization::DataStructs::AUTH_REASON_AUTHENTICATED);
                }
            }
            else
            {
                extraInfoOut["auth"][std::to_string(passIdx)]["reasonTxt"] = getAuthReasonText(authReason);
                extraInfoOut["auth"][std::to_string(passIdx)]["reasonVal"] = static_cast<Json::UInt>(authReason);
            }
        }

        auto authorizer = authDomains->openDomain(session->getAuthDomain());
        if (authorizer)
        {
            Json::Value payloadOut;
            Json::Value reasons;
            auto i = methodsManager->validateRPCMethodPerms( authorizer,  session, request.getMethodName(), extraTmpIndexes, &reasons);
            authDomains->closeDomain(session->getAuthDomain());

            switch (i)
            {
            case XRPC::VALIDATION_OK:
            {
                response.setRetCode(methodsManager->runRPCMethod(authDomains, sessionId, session, request.getMethodName(), request.getPayload(), request.getExtraInfo(), &payloadOut, &extraInfoOut ));
                httpResponseCode = Network::Parsers::HTTP_RET_200_OK;
            }break;
            case XRPC::VALIDATION_NOTAUTHORIZED:
            {
                // not enough permissions.
                extraInfoOut["auth_reasons"] = reasons;
                response.setRetCode(XRPC::METHOD_RET_CODE_INVALIDAUTH);
                httpResponseCode = Network::Parsers::HTTP_RET_401_UNAUTHORIZED;
            }break;
            case XRPC::VALIDATION_METHODNOTFOUND:
            default:
            {
                // not enough permissions.
                response.setRetCode(XRPC::METHOD_RET_CODE_METHODNOTFOUND);
                httpResponseCode = Network::Parsers::HTTP_RET_404_NOT_FOUND;
            }break;
            }
        }
        else
        {
            response.setRetCode(XRPC::METHOD_RET_CODE_INVALIDDOMAIN);
            httpResponseCode = Network::Parsers::HTTP_RET_403_FORBIDDEN;
        }

        if (deleteSession)
        {
            ////////////////////////////////////
            // Temporary session mgmt
            ////////////////////////////////////
            delete session;
            session = nullptr;
        }

    }

    // QUIT: Should be authenticated in a persistent fashion...
    if ( session && request.getRpcMode() == "QUIT" )
    {
        response.setRetCode(XRPC::METHOD_RET_CODE_SUCCESS);
        destroySession = true;
    }

    // Close the openned session.
    if (closeSession) sessionsManagger->closeSession(sessionId);
    if (destroySession) sessionsManagger->destroySession(sessionId);

    // Setup the response...
    response.setExtraInfo(extraInfoOut);
    response.setPayload(payloadOut);

    // Send the response to the client
    sendAnswer(&response,out);

    // return the HTTP response code.
    return httpResponseCode;
}

void ClientHandler::setSessionsManagger(SessionsManager *value)
{
    sessionsManagger = value;
}

void ClientHandler::setRemoteIP(const std::string &value)
{
    remoteIP = value;
}

Authorization::Session::IAuth_Session * ClientHandler::persistentAuthentication(const string &userName, const string &domainName, const XRPC::Authentication &authData, Authorization::Session::IAuth_Session *session, Authorization::DataStructs::AuthReason * authReason)
{
    if (!session && authData.getPassIndex()!=0) return nullptr;

    *authReason = Authorization::DataStructs::AUTH_REASON_INVALID_DOMAIN;
    std::string sessionId;
    Json::Value payload;

    auto auth = authDomains->openDomain(domainName);
    if (auth)
    {
        *authReason = auth->authenticate(userName,authData.getUserPass(),authData.getPassIndex());
        authDomains->closeDomain(domainName);
    }

    if (!session && *authReason == Authorization::DataStructs::AUTH_REASON_AUTHENTICATED)
    {
        session  = new Authorization::Session::IAuth_Session;
        session->registerPersistentAuthentication(userName,domainName,authData.getPassIndex(),*authReason);
        sessionsManagger->addSession(session);
    }

    return session;
}

Authorization::DataStructs::AuthReason ClientHandler::temporaryAuthentication(const XRPC::Authentication &authData, Authorization::Session::IAuth_Session *session)
{
    Authorization::DataStructs::AuthReason reason;
    if (!session) return Authorization::DataStructs::AUTH_REASON_UNAUTHENTICATED;

    std::string userName = session->getAuthUser();
    std::string domainName = session->getAuthDomain();

    auto auth = authDomains->openDomain(domainName);
    if (!auth)
        reason = Authorization::DataStructs::AUTH_REASON_INVALID_DOMAIN;
    else
    {
        reason = auth->authenticate( userName,authData.getUserPass(),authData.getPassIndex()); // Authenticate in a non-persistent fashion.
        authDomains->closeDomain(domainName);
    }

    return reason;
}

void ClientHandler::sendAnswer(RPC::XRPC::Request *response, Memory::Streams::Streamable *out)
{
    Memory::Streams::Status stat;
    Memory::Streams::JSON_Streamable outJStreamable;
    outJStreamable.setFormatted(useFormattedJSONOutput);
    Json::Value * outValue = outJStreamable.getValue();
    *outValue = response->toJSON();
    outJStreamable.streamTo(out,stat);
}

bool ClientHandler::getUseFormattedJSONOutput() const
{
    return useFormattedJSONOutput;
}

void ClientHandler::setUseFormattedJSONOutput(bool value)
{
    useFormattedJSONOutput = value;
}

void ClientHandler::setMethodsManager(XRPC::MethodsManager *value)
{
    methodsManager = value;
}
