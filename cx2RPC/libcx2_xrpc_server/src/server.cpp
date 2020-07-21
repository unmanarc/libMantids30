#include "server.h"

#include <cx2_xrpc_common/retcodes.h>

using namespace CX2::RPC::XRPC;
using namespace CX2;

Server::Server(Memory::Streams::Streamable *sobject) : Memory::Streams::Parsing::Parser(sobject,false)
{
    // Set current parser for .parse() function
    currentParser = (Memory::Streams::Parsing::SubParser *)(&remoteHandshake);

    bIsValidHandShake = false;

    // Create a thread for dequeing replies
    request.initElemParser(sobject,false);
    localHandshake.initElemParser(sobject,false);
    remoteHandshake.initElemParser(sobject,false);
}

Server::~Server()
{
}

void Server::setAuthenticators(Authorization::IAuth_Domains *authenticator)
{
    authDomains = authenticator;
}


Authorization::Session::IAuth_Session *Server::getAuthSession()
{
    return &session;
}

bool Server::sendAnswer(Request & response)
{
    std::unique_lock<std::mutex> lock(mutex_send);
    Memory::Streams::Status wrStat;
    response.initElemParser(streamableObject,false);
    return response.stream(wrStat);
}

bool Server::initProtocol()
{
    std::unique_lock<std::mutex> lock(mutex_send);
    Memory::Streams::Status wrStat;
    // server send handshake...;
    return localHandshake.stream(wrStat);
}

bool Server::changeToNextParser()
{
    if (currentParser == &remoteHandshake)
    {
        // Validate handshake...
        if (validateHandshake())
        {
            bIsValidHandShake = true;
            currentParser = &request;
        }
        else
        {
            currentParser = nullptr; // END
            bIsValidHandShake = false;
        }
    }
    else if (currentParser == &request)
    {
        // process request...
        if (processRequest())
            request.clear();
        else
            currentParser = nullptr; // END
    }
    return true;
}

bool Server::validateHandshake()
{
    if (remoteHandshake.getProtocolVersion()!=localHandshake.getProtocolVersion())
        return false;
    return true;
}

bool Server::processRequest()
{
    /*    std::cout << "Receiving from client: -------------------------------------" << std::endl << std::flush;
    request.print();
    std::cout << "------------------------------------------------------------" << std::endl << std::flush;*/
    if (request.getRpcMode() == "QUIT")
        return false;
    else if (request.getRpcMode() == "AUTH")
    {
        // Login for the first time...
        if (request.getMethodName() == "LOGIN")
        {
            Authentication auth = request.getAuthentication(0);
            Json::Value authPayload = request.getPayload();
            if (!authPayload["user"].isString() ||  !authPayload["domain"].isString())
                return false;
            persistentAuthentication(authPayload["user"].asString(),authPayload["domain"].asString(),auth);
        }
    }
    else if (request.getRpcMode() == "EXEC")
    {
        Request answer;
        answer.setReqId(request.getReqId());
        answer.setMethodName(request.getMethodName());
        answer.setRpcMode("EXEC");

        // First: take authentications...
        std::set<uint32_t> extraTmpIndexes;
        for (const uint32_t & passIdx : request.getAuthenticationsIdxs())
        {
            if (temporaryAuthentication(request.getAuthentication(passIdx)))
            {
                extraTmpIndexes.insert(passIdx);
            }
            else
                return true; // bad auth.
        }

        Json::Value reasons;

        auto auth = authDomains->openDomain(session.getAuthDomain());
        if (auth)
        {
            auto i = methodsManager->validateRPCMethodPerms( auth,  &session, request.getMethodName(), extraTmpIndexes, &reasons);
            authDomains->closeDomain(session.getAuthDomain());

            switch (i)
            {
            case VALIDATION_OK:
            {
            }break;
            case VALIDATION_NOTAUTHORIZED:
            {
                // not enough permissions.
                answer.setExtraInfo(reasons);
                answer.setRetCode(METHOD_RET_CODE_INVALIDAUTH);
                return sendAnswer( answer );
            }
            case VALIDATION_METHODNOTFOUND:
            {
                // not enough permissions.
                answer.setRetCode(METHOD_RET_CODE_METHODNOTFOUND);
                return sendAnswer( answer );
            }
            }
            sRPCParameters * rpcParameters = new sRPCParameters;
            if (rpcParameters)
            {
                rpcParameters->domainName = session.getAuthDomain();
                rpcParameters->authDomains = authDomains;
                rpcParameters->session = &session;
                rpcParameters->extraInfo = request.getExtraInfo();
                rpcParameters->methodName = request.getMethodName();
                rpcParameters->payload = request.getPayload();
                rpcParameters->connectionSender = this;
                rpcParameters->rpcMethodsCaller = methodsManager;
                rpcParameters->requestId = request.getReqId();

                if (!methodsManager->pushRPCMethodIntoQueue(rpcParameters, session.getAuthUser()))
                {
                    answer.setRetCode(METHOD_RET_CODE_TIMEDOUT);
                    delete rpcParameters;
                    return sendAnswer( answer );
                }
            }
            else
            {
                // Memory full
                answer.setRetCode(METHOD_RET_CODE_SERVERMEMORYFULL);
                return sendAnswer( answer );
            }
        }
        else
        {
            answer.setRetCode(METHOD_RET_CODE_INVALIDDOMAIN);
            return sendAnswer( answer );
        }

    }
    return true;
}

bool Server::temporaryAuthentication(const Authentication &authData)
{
    Authorization::DataStructs::AuthReason reason;
    Json::Value payload, extraInfo;

    auto auth = authDomains->openDomain(session.getAuthDomain());
    if (!auth)
        reason = Authorization::DataStructs::AUTH_REASON_INVALID_DOMAIN;
    else
    {
        reason = auth->authenticate( session.getAuthUser(),authData.getUserPass(),authData.getPassIndex()); // Authenticate in a non-persistent fashion.
        authDomains->closeDomain(session.getAuthDomain());
    }

    if ( reason != Authorization::DataStructs::AUTH_REASON_AUTHENTICATED )
    {
        extraInfo["reasonTxt"] = getAuthReasonText(reason);
        extraInfo["reasonVal"] = reason;
        extraInfo["passIndex"] = authData.getPassIndex();

        Request response;
        response.setRpcMode("EXEC");
        response.setMethodName(request.getMethodName());
        response.setReqId(request.getReqId());
        response.setExtraInfo(extraInfo);
        response.setRetCode(METHOD_RET_CODE_INVALIDLOCALAUTH);

        sendAnswer(response);

        return false;
    }
    return true;
}

void Server::persistentAuthentication(const std::string &user, const std::string &domain, const Authentication &authData)
{
    Json::Value payload;
    Authorization::DataStructs::AuthReason authReason;

    auto auth = authDomains->openDomain(domain);
    if (auth)
    {
        authReason = auth->authenticate(user,authData.getUserPass(),authData.getPassIndex());
        authDomains->closeDomain(domain);
        session.registerPersistentAuthentication(user,domain,authData.getPassIndex(),authReason);
    }
    else
        authReason = Authorization::DataStructs::AUTH_REASON_INVALID_DOMAIN;

    payload["reasonTxt"] = getAuthReasonText(authReason);
    payload["reasonVal"] = static_cast<Json::UInt>(authReason);
    payload["passIndex"] = authData.getPassIndex();

    Request response;
    response.setRpcMode("AUTH");
    response.setMethodName("LOGIN");
    response.setPayload(payload);

    sendAnswer(response);
}

void Server::setMethodsManager(MethodsManager *value)
{
    methodsManager = value;
}

bool Server::isValidHandShake() const
{
    return bIsValidHandShake;
}

Handshake * Server::getRemoteHandshake()
{
    return &remoteHandshake;
}

Handshake * Server::getLocalHandshake()
{
    return &localHandshake;
}
