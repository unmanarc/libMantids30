#include <Mantids30/DataFormat_JWT/jwt.h>
#include "fastrpc3.h"
#include <Mantids30/API_Monolith/methodshandler.h>
#include <Mantids30/Helpers/callbacks.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Net_Sockets/acceptor_multithreaded.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <Mantids30/Threads/lock_shared.h>

#include <boost/algorithm/string/predicate.hpp>

using namespace Mantids30;
using namespace Network::Sockets;
using namespace Network::Protocols::FastRPC;
using namespace std;

using Ms = chrono::milliseconds;
using S = chrono::seconds;

void FastRPC3::LocalRPCTasks::executeLocalTask(void *vTaskParams)
{
    bool functionFound = false;

    TaskParameters *taskParams = (TaskParameters *) (vTaskParams);
    CallbackDefinitions *callbacks = ((CallbackDefinitions *) taskParams->callbacks);
    std::shared_ptr<Sessions::Session> session = taskParams->sessionHolder->getSharedPointer();

    json fullResponse;
    json responsePayload;
    fullResponse["statusCode"] = ELT_RET_SUCCESS;

    Helpers::JSONReader2 reader;
    bool sessionFailed = false;
    DataFormat::JWT::Token extraJWT;

    // Process the extra token
    if (taskParams->extraTokenAuth)
    {
        extraJWT = taskParams->jwtValidator->verifyAndDecodeTokenPayload(taskParams->extraTokenAuth);
        if (extraJWT.isValid())
        {
            // valid extra token...
            if (extraJWT.getImpersonator().empty())
            {
                // if no session, and the token is not for impersonation, use as session token.
                if (!session)
                {
                    // there is no session... by example, the method does not require active session
                    // <<< SESSION DOES NOT EXIST HERE...
                    // Create a new virtual session here:
                    session = std::make_shared<Sessions::Session>(extraJWT);
                    // Set the token info/claims...
                    //session->setJWTAuthenticatedInfo(extraJWT);
                }
                else
                {
                    sessionFailed = true;
                    CALLBACK(callbacks->onTokenValidationFailure)(callbacks->context, taskParams, taskParams->extraTokenAuth, CallbackDefinitions::EXTRATOKEN_NOTREQUIRED_ERROR);
                    fullResponse["statusCode"] = ELT_RET_TOKENFAILED;
                }
            }
            else
            {
                // If it's impersonation, check that the impersonator is the current session owner, and temporary change the session...
                if (session->getUser() == extraJWT.getImpersonator() && extraJWT.getDomain() == session->getDomain())
                {
                    // Create a new impersonated session here:
                    session = std::make_shared<Sessions::Session>(extraJWT);
                    // Set the token info/claims...
                    //session->setJWTAuthenticatedInfo(extraJWT);
                }
                else
                {
                    // report the problem...
                    // This is not an impersonator token.
                    sessionFailed = true;
                    CALLBACK(callbacks->onTokenValidationFailure)(callbacks->context, taskParams, taskParams->extraTokenAuth, CallbackDefinitions::EXTRATOKEN_IMPERSONATION_ERROR);
                    fullResponse["statusCode"] = ELT_RET_INVALIDIMPERSONATOR;
                }
            }
        }
        else
        {
            sessionFailed = true;
            CALLBACK(callbacks->onTokenValidationFailure)(callbacks->context, taskParams, taskParams->extraTokenAuth, CallbackDefinitions::EXTRATOKEN_VALIDATION_ERROR);
            fullResponse["statusCode"] = ELT_RET_TOKENFAILED;
        }
    }

    // Invalidate current revoked sessions...
    if (!sessionFailed && session && session->isSessionRevoked())
    {
        sessionFailed = true;
        session = nullptr;
        CALLBACK(callbacks->onTokenValidationFailure)(callbacks->context, taskParams, taskParams->extraTokenAuth, CallbackDefinitions::TOKEN_REVOKED);
        fullResponse["statusCode"] = ELT_RET_TOKENFAILED;
    }

    if (  !taskParams->methodsHandler->doesMethodRequireActiveSession(taskParams->methodName) // method does not require session.
        || // OR
        (taskParams->methodsHandler->doesMethodRequireActiveSession(taskParams->methodName) && session) // method require session and there is session.
        )
    {
        // There is an extra authentication token and session is OK.
        if (!sessionFailed)
        {
            json reasons;

            auto i = taskParams->methodsHandler->validateMethodRequirements(session, taskParams->methodName, &reasons);

            switch (i)
            {
            case API::Monolith::MethodsHandler::VALIDATION_OK:
            {
                if (session)
                    session->updateLastActivity();

                // Report:
                CALLBACK(callbacks->onMethodExecutionStart)(callbacks->context, taskParams, taskParams->payload);

                auto start = chrono::high_resolution_clock::now();
                auto finish = chrono::high_resolution_clock::now();
                chrono::duration<double, milli> elapsed = finish - start;

                switch (taskParams->methodsHandler->invoke(session, taskParams->methodName, taskParams->payload, &responsePayload))
                {
                case API::Monolith::MethodsHandler::METHOD_RET_CODE_SUCCESS:

                    finish = chrono::high_resolution_clock::now();
                    elapsed = finish - start;

                    CALLBACK(callbacks->onMethodExecutionSuccess)(callbacks->context, taskParams, elapsed.count(), responsePayload);

                    functionFound = true;
                    fullResponse["statusCode"] = 200;
                    break;
                case API::Monolith::MethodsHandler::METHOD_RET_CODE_METHODNOTFOUND:

                    CALLBACK(callbacks->onMethodExecutionNotFound)(callbacks->context, taskParams);
                    fullResponse["statusCode"] = ELT_RET_METHODNOTIMPLEMENTED;
                    break;
                default:
                    CALLBACK(callbacks->onMethodExecutionUnknownError)(callbacks->context, taskParams);
                    fullResponse["statusCode"] = ELT_RET_INTERNALERROR;
                    break;
                }
            }
            break;
            case API::Monolith::MethodsHandler::VALIDATION_NOTAUTHORIZED:
            {
                // not enough permissions.
                CALLBACK(callbacks->onMethodExecutionNotAuthorized)(callbacks->context, taskParams, reasons);
                fullResponse["auth"]["reasons"] = reasons;
                fullResponse["statusCode"] = ELT_RET_NOTAUTHORIZED;
            }
            break;
            case API::Monolith::MethodsHandler::VALIDATION_METHODNOTFOUND:
            default:
            {
                CALLBACK(callbacks->onMethodExecutionNotFound)(callbacks->context, taskParams);
                fullResponse["statusCode"] = ELT_RET_METHODNOTIMPLEMENTED;
            }
            break;
            }
        }
    }
    else
    {
        CALLBACK(callbacks->onMethodExecutionSessionMissing)(callbacks->context, taskParams);
        fullResponse["statusCode"] = ELT_RET_REQSESSION;
    }


    //Json::StreamWriterBuilder builder;
    //builder.settings_["indentation"] = "";

    //
    fullResponse["payload"] = responsePayload;
    sendRPCAnswer(taskParams, fullResponse.toStyledString(), functionFound ? 2 : 4);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC3::LocalRPCTasks::getSSOData(void *taskData)
{
    FastRPC3::TaskParameters *taskParams = (FastRPC3::TaskParameters *) (taskData);
    FastRPC3 * caller = (FastRPC3 *)taskParams->caller;

    json data;
    data["loginURL"] = caller->config.loginURL;
    data["returnURI"] = caller->config.returnURI;
    data["ignoreSSLCertForSSO"] = caller->config.ignoreSSLCertForSSO;

    sendRPCAnswer(taskParams, data.toStyledString(), 2);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC3::LocalRPCTasks::login(void *taskData)
{
    FastRPC3::TaskParameters *taskParams = (FastRPC3::TaskParameters *) (taskData);
    CallbackDefinitions *callbacks = ((CallbackDefinitions *) taskParams->callbacks);

    // CREATE NEW SESSION:
    json response;
    LoginReason loginReason;

    auto session = taskParams->sessionHolder->getSharedPointer();

    std::string sJWTToken = JSON_ASSTRING(taskParams->payload, "jwtToken", "");

    if (session)
    {
        // Close the session before.
        loginReason.reason = LoginReason::SESSION_DUPLICATE;
    }
    else
    {
        // PROCEED THEN....
        DataFormat::JWT::Token token = taskParams->jwtValidator->verifyAndDecodeTokenPayload(sJWTToken);
        if (token.isValid())
        {
            session = taskParams->sessionHolder->create(token);
            if (session)
            {
                //session->setUser(token.getSubject());
                session->updateLastActivity();

                loginReason.reason = LoginReason::TOKEN_VALIDATED;
                CALLBACK(callbacks->onTokenValidationSuccess)(callbacks->context, taskParams, sJWTToken);
            }
            else
            {
                loginReason.reason = LoginReason::INTERNAL_ERROR;
                CALLBACK(callbacks->onMethodExecutionUnknownError)(callbacks->context, taskParams);
            }
        }
        else
        {
            loginReason.reason = LoginReason::TOKEN_FAILED;
            CALLBACK(callbacks->onTokenValidationFailure)(callbacks->context, taskParams, sJWTToken, CallbackDefinitions::TOKEN_VALIDATION_ERROR);
        }
    }

    response = loginReason.toJsonResponse();
    sendRPCAnswer(taskParams, response.toStyledString(), 2);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC3::LocalRPCTasks::logout(void *taskData)
{
    FastRPC3::TaskParameters *params = (FastRPC3::TaskParameters *) (taskData);
    json response;
    response = params->sessionHolder->destroy();
    sendRPCAnswer(params, response.toStyledString(), 2);
    params->doneSharedMutex->unlockShared();
}
