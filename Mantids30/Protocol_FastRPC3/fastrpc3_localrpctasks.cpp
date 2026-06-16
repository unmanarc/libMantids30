#include "fastrpc3.h"
#include <Mantids30/API_EndpointsAndSessions/api_monolith_endpoints.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Helpers/callbacks.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <Mantids30/Threads/lock_shared.h>

#include <boost/algorithm/string/predicate.hpp>
#include <chrono>
#include <memory>

using namespace Mantids30;
using namespace Network::Sockets;
using namespace Network::Protocol::FastRPC;
using namespace Mantids30::DataFormat;
using namespace std;

using Ms = chrono::milliseconds;
using S = chrono::seconds;

void FastRPC3::LocalRPCTasks::executeLocalTask(const std::shared_ptr<void>& vTaskParams)
{
    bool functionFound = false;

    TaskParameters *taskParams = (TaskParameters *) (vTaskParams.get());
    RPC3CallbackDefinitions *callbacks = ((RPC3CallbackDefinitions *) taskParams->callbacks);
    std::shared_ptr<Sessions::Session> session = taskParams->sessionHolder->getSharedPointer();

    json fullResponse;
    json responsePayload;
    fullResponse["statusCode"] = static_cast<uint16_t>(LocalTaskExecutionResult::SUCCESS);

    Helpers::JSONReader2 reader;
    bool sessionFailed = false;
    JWT::Token extraJWT;

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
                    CALLBACK(callbacks->onTokenValidationFailure)(callbacks->context, taskParams, taskParams->extraTokenAuth, RPC3CallbackDefinitions::TokenValidationStatus::EXTRATOKEN_NOTREQUIRED_ERROR);
                    fullResponse["statusCode"] = static_cast<uint16_t>(LocalTaskExecutionResult::INVALID_TOKEN);
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
                    CALLBACK(callbacks->onTokenValidationFailure)(callbacks->context, taskParams, taskParams->extraTokenAuth, RPC3CallbackDefinitions::TokenValidationStatus::EXTRATOKEN_IMPERSONATION_ERROR);
                    fullResponse["statusCode"] = static_cast<uint16_t>(LocalTaskExecutionResult::INVALID_IMPERSONATOR_TOKEN);
                }
            }
        }
        else
        {
            sessionFailed = true;
            CALLBACK(callbacks->onTokenValidationFailure)(callbacks->context, taskParams, taskParams->extraTokenAuth, RPC3CallbackDefinitions::TokenValidationStatus::EXTRATOKEN_SIGNATURE_ERROR);
            fullResponse["statusCode"] = static_cast<uint16_t>(LocalTaskExecutionResult::INVALID_TOKEN);
        }
    }

    // Invalidate current revoked sessions...
    if (!sessionFailed && session && session->isSessionRevoked())
    {
        sessionFailed = true;
        session = nullptr;
        CALLBACK(callbacks->onTokenValidationFailure)(callbacks->context, taskParams, taskParams->extraTokenAuth, RPC3CallbackDefinitions::TokenValidationStatus::SESSION_REVOKED_ERROR);
        fullResponse["statusCode"] = static_cast<uint16_t>(LocalTaskExecutionResult::INVALID_TOKEN);
    }

    if (!taskParams->methodsHandler->doesAPIEndpointRequireActiveSession(taskParams->methodName)             // method does not require session.
        ||                                                                                                   // OR
        (taskParams->methodsHandler->doesAPIEndpointRequireActiveSession(taskParams->methodName) && session) // method require session and there is session.
    )
    {
        // There is an extra authentication token and session is OK.
        if (!sessionFailed)
        {
            json reasons;

            API::Monolith::Endpoints::ValidationResult i = taskParams->methodsHandler->validateEndpointRequirements(session, taskParams->methodName, &reasons);

            switch (i)
            {
            case API::Monolith::Endpoints::ValidationResult::SUCCESS:
            {
                if (session)
                {
                    session->updateLastActivity();
                }

                // Report:
                CALLBACK(callbacks->onMethodExecutionStart)(callbacks->context, taskParams, taskParams->payload);

                chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
                chrono::high_resolution_clock::time_point finish = chrono::high_resolution_clock::now();
                chrono::duration<double, milli> elapsed = finish - start;

                switch (taskParams->methodsHandler->invoke(session, taskParams->methodName, taskParams->payload, &responsePayload))
                {
                case API::Monolith::Endpoints::StatusCode::SUCCESS:

                    finish = chrono::high_resolution_clock::now();
                    elapsed = finish - start;

                    CALLBACK(callbacks->onMethodExecutionSuccess)(callbacks->context, taskParams, elapsed.count(), responsePayload);

                    functionFound = true;
                    fullResponse["statusCode"] = 200;
                    break;
                case API::Monolith::Endpoints::StatusCode::NOTFOUND:

                    CALLBACK(callbacks->onMethodExecutionNotFound)(callbacks->context, taskParams);
                    fullResponse["statusCode"] = static_cast<uint16_t>(LocalTaskExecutionResult::METHOD_NOT_IMPLEMENTED);
                    break;
                default:
                    CALLBACK(callbacks->onMethodExecutionUnknownError)(callbacks->context, taskParams);
                    fullResponse["statusCode"] = static_cast<uint16_t>(LocalTaskExecutionResult::INTERNAL_ERROR);
                    break;
                }
            }
            break;
            case API::Monolith::Endpoints::ValidationResult::NOTAUTHORIZED:
            {
                // not authorized.
                CALLBACK(callbacks->onMethodExecutionNotAuthorized)(callbacks->context, taskParams, reasons);
                fullResponse["auth"]["reasons"] = reasons;
                fullResponse["statusCode"] = static_cast<uint16_t>(LocalTaskExecutionResult::NOT_AUTHORIZED);
            }
            break;
            case API::Monolith::Endpoints::ValidationResult::ENDPOINTNOTFOUND:
            default:
            {
                CALLBACK(callbacks->onMethodExecutionNotFound)(callbacks->context, taskParams);
                fullResponse["statusCode"] = static_cast<uint16_t>(LocalTaskExecutionResult::METHOD_NOT_IMPLEMENTED);
            }
            break;
            }
        }
    }
    else
    {
        CALLBACK(callbacks->onMethodExecutionSessionMissing)(callbacks->context, taskParams);
        fullResponse["statusCode"] = static_cast<uint16_t>(LocalTaskExecutionResult::REQUIRED_SESSION_NOT_FOUND);
    }

    //Json::StreamWriterBuilder builder;
    //builder.settings_["indentation"] = "";

    //
    fullResponse["payload"] = responsePayload;
    sendRPCAnswer(taskParams, fullResponse.toStyledString(), functionFound ? EXEC_STATUS_SUCCESS : EXEC_STATUS_ERR_METHOD_NOT_FOUND);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC3::LocalRPCTasks::getSSOData(const std::shared_ptr<void> & taskData)
{
    FastRPC3::TaskParameters *taskParams = static_cast<FastRPC3::TaskParameters *>(taskData.get());
    FastRPC3 *caller = static_cast<FastRPC3 *>(taskParams->caller);

    json data;
    data["loginURL"] = caller->config.loginURL;
    data["returnURI"] = caller->config.returnURI;
    data["ignoreSSLCertForSSO"] = caller->config.ignoreSSLCertForSSO;

    sendRPCAnswer(taskParams, data.toStyledString(), EXEC_STATUS_SUCCESS);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC3::LocalRPCTasks::login(const std::shared_ptr<void>& taskData)
{
    FastRPC3::TaskParameters *taskParams = static_cast<FastRPC3::TaskParameters *>(taskData.get());
    RPC3CallbackDefinitions *callbacks = static_cast<RPC3CallbackDefinitions *>(taskParams->callbacks);

    // CREATE NEW SESSION:
    json response;
    LoginAuthentication loginAuthResult;

    std::shared_ptr<Sessions::Session> session = taskParams->sessionHolder->getSharedPointer();

    std::string sJWTToken = JSON_ASSTRING(taskParams->payload, "jwtToken", "");

    if (session)
    {
        // Close the session before.
        loginAuthResult.result = LoginAuthentication::Result::SESSION_DUPLICATE;
    }
    else
    {
        // PROCEED THEN....
        JWT::Token token = taskParams->jwtValidator->verifyAndDecodeTokenPayload(sJWTToken);
        if (token.isValid())
        {
            session = taskParams->sessionHolder->create(token);
            if (session)
            {
                //session->setUser(token.getSubject());
                session->updateLastActivity();

                loginAuthResult.result = LoginAuthentication::Result::TOKEN_VALIDATED;
                CALLBACK(callbacks->onTokenValidationSuccess)(callbacks->context, taskParams, sJWTToken);
            }
            else
            {
                loginAuthResult.result = LoginAuthentication::Result::INTERNAL_ERROR;
                CALLBACK(callbacks->onMethodExecutionUnknownError)(callbacks->context, taskParams);
            }
        }
        else
        {
            loginAuthResult.result = LoginAuthentication::Result::TOKEN_FAILED;
            CALLBACK(callbacks->onTokenValidationFailure)(callbacks->context, taskParams, sJWTToken, RPC3CallbackDefinitions::TokenValidationStatus::SIGNATURE_ERROR);
        }
    }

    response = loginAuthResult.toJSONResponse();
    sendRPCAnswer(taskParams, response.toStyledString(), EXEC_STATUS_SUCCESS);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC3::LocalRPCTasks::logout(const std::shared_ptr<void> & taskData)
{
    FastRPC3::TaskParameters *params = static_cast<FastRPC3::TaskParameters *>(taskData.get());
    json response;
    response = params->sessionHolder->destroy();
    sendRPCAnswer(params, response.toStyledString(), EXEC_STATUS_SUCCESS);
    params->doneSharedMutex->unlockShared();
}
