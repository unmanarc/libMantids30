#include "Mantids30/DataFormat_JWT/jwt.h"
#include "fastrpc3.h"
#include <Mantids30/API_Monolith/methodshandler.h>
#include <Mantids30/Auth/ds_authentication.h>
#include <Mantids30/Auth/multicredentialdata.h>
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
    std::shared_ptr<Auth::Session> session = taskParams->sessionHolder->getSharedPointer();

    json fullResponse;
    json responsePayload;
    fullResponse["statusCode"] = ELT_RET_SUCCESS;

    Helpers::JSONReader2 reader;
    bool errorInExtraToken = false;
    std::string effectiveUserName;

    // TODO: copy this to monolith...
    // Invalidate bad sessions...
    if (session && !session->validateSession())
    {
        errorInExtraToken = true;
        session = nullptr;
        CALLBACK(callbacks->CB_TokenValidation_Failed)(callbacks->obj, taskParams, taskParams->extraTokenAuth, CallbackDefinitions::TOKEN_REVOKED);
        fullResponse["statusCode"] = ELT_RET_TOKENFAILED;
    }

    // If there is a session, overwrite the user/domain inputs...
    if (session)
    {
        effectiveUserName = session->getEffectiveUser();
    }    

    if (!taskParams->methodsHandler->doesMethodRequireActiveSession(taskParams->methodName) || (taskParams->methodsHandler->doesMethodRequireActiveSession(taskParams->methodName) && session))
    {
        if (taskParams->extraTokenAuth && !errorInExtraToken)
        {
            // EXTRA TOKEN...
            DataFormat::JWT::Token token = taskParams->jwtValidator->verifyAndDecodeTokenPayload(taskParams->extraTokenAuth);
            if (token.isValid())
            {
                if (!session)
                {
                    // <<< SESSION DOES NOT EXIST HERE...
                    // Create a new session here:
                    session = std::make_shared<Auth::Session>();
                    effectiveUserName = token.getSubject();
                    session->setAuthenticatedUser(effectiveUserName);
                    session->setClaims(token.getAllClaims());
                }
                else
                {
                    // <<< PREVIOUS SESSION EXIST HERE... SET THE NEW TOKEN CLAIMS
                    // Copy the session in a new context (create new session for this)...
                    session = std::make_shared<Auth::Session>(session);
                    // Set new claims:
                    session->setClaims(token.getAllClaims());

                    // If you want to impersonate, you need to send the impersonation token to each request...
                    if (token.getSubject() != effectiveUserName)
                    {
                        // <<< THIS IS AN IMPERSONATION TOKEN...
                        if (JSON_ASSTRING_D(token.getClaim("impersonator"), "") == effectiveUserName)
                        {
                            session->setImpersonatedUser(token.getSubject());
                            effectiveUserName = session->getEffectiveUser();
                        }
                        else
                        {
                            // This is not an impersonator token.
                            errorInExtraToken = true;
                            CALLBACK(callbacks->CB_TokenValidation_Failed)(callbacks->obj, taskParams, taskParams->extraTokenAuth, CallbackDefinitions::EXTRATOKEN_IMPERSONATION_ERROR);
                            fullResponse["statusCode"] = ELT_RET_INVALIDIMPERSONATOR;
                        }
                    }
                }
            }
            else
            {
                errorInExtraToken = true;
                CALLBACK(callbacks->CB_TokenValidation_Failed)(callbacks->obj, taskParams, taskParams->extraTokenAuth, CallbackDefinitions::EXTRATOKEN_VALIDATION_ERROR);
                fullResponse["statusCode"] = ELT_RET_TOKENFAILED;
            }
        }

        if (!errorInExtraToken)
        {
            json reasons;

            auto i = taskParams->methodsHandler->validateMethodRequirements(session.get(), taskParams->methodName, &reasons);

            switch (i)
            {
            case API::Monolith::MethodsHandler::VALIDATION_OK:
            {
                if (session)
                    session->updateLastActivity();

                // Report:
                CALLBACK(callbacks->CB_MethodExecution_Starting)(callbacks->obj, taskParams, taskParams->payload);

                auto start = chrono::high_resolution_clock::now();
                auto finish = chrono::high_resolution_clock::now();
                chrono::duration<double, milli> elapsed = finish - start;

                switch (taskParams->methodsHandler->invoke(session.get(), taskParams->methodName, taskParams->payload, &responsePayload))
                {
                case API::Monolith::MethodsHandler::METHOD_RET_CODE_SUCCESS:

                    finish = chrono::high_resolution_clock::now();
                    elapsed = finish - start;

                    CALLBACK(callbacks->CB_MethodExecution_ExecutedOK)(callbacks->obj, taskParams, elapsed.count(), responsePayload);

                    functionFound = true;
                    fullResponse["statusCode"] = 200;
                    break;
                case API::Monolith::MethodsHandler::METHOD_RET_CODE_METHODNOTFOUND:

                    CALLBACK(callbacks->CB_MethodExecution_MethodNotFound)(callbacks->obj, taskParams);
                    fullResponse["statusCode"] = ELT_RET_METHODNOTIMPLEMENTED;
                    break;
                default:
                    CALLBACK(callbacks->CB_MethodExecution_UnknownError)(callbacks->obj, taskParams);
                    fullResponse["statusCode"] = ELT_RET_INTERNALERROR;
                    break;
                }
            }
            break;
            case API::Monolith::MethodsHandler::VALIDATION_NOTAUTHORIZED:
            {
                // not enough permissions.
                CALLBACK(callbacks->CB_MethodExecution_NotAuthorized)(callbacks->obj, taskParams, reasons);
                fullResponse["auth"]["reasons"] = reasons;
                fullResponse["statusCode"] = ELT_RET_NOTAUTHORIZED;
            }
            break;
            case API::Monolith::MethodsHandler::VALIDATION_METHODNOTFOUND:
            default:
            {
                CALLBACK(callbacks->CB_MethodExecution_MethodNotFound)(callbacks->obj, taskParams);
                fullResponse["statusCode"] = ELT_RET_METHODNOTIMPLEMENTED;
            }
            break;
            }
        }
    }
    else
    {
        CALLBACK(callbacks->CB_MethodExecution_RequiredSessionNotProvided)(callbacks->obj, taskParams);
        fullResponse["statusCode"] = ELT_RET_REQSESSION;
    }


    //Json::StreamWriterBuilder builder;
    //builder.settings_["indentation"] = "";

    //
    fullResponse["payload"] = responsePayload;
    sendRPCAnswer(taskParams, fullResponse.toStyledString(), functionFound ? 2 : 4);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC3::LocalRPCTasks::login(void *taskData)
{
    FastRPC3::TaskParameters *taskParams = (FastRPC3::TaskParameters *) (taskData);
    CallbackDefinitions *callbacks = ((CallbackDefinitions *) taskParams->callbacks);

    // CREATE NEW SESSION:
    json response;
    Auth::Reason authReason = Auth::REASON_INTERNAL_ERROR;

    auto session = taskParams->sessionHolder->getSharedPointer();

    std::string sJWTToken = JSON_ASSTRING(taskParams->payload, "jwtToken", "");

    if (session)
    {
        // Close the session before.
        authReason = Auth::REASON_DUPLICATED_SESSION;
    }
    else
    {
        // PROCEED THEN....
        DataFormat::JWT::Token token = taskParams->jwtValidator->verifyAndDecodeTokenPayload(sJWTToken);
        if (token.isValid())
        {
            session = taskParams->sessionHolder->create();
            if (session)
            {
                session->setAuthenticatedUser(token.getSubject());
                session->updateLastActivity();

                authReason = Auth::REASON_AUTHENTICATED;
                CALLBACK(callbacks->CB_TokenValidation_OK)(callbacks->obj, taskParams, sJWTToken);
            }
            else
            {
                authReason = Auth::REASON_INTERNAL_ERROR;
                CALLBACK(callbacks->CB_MethodExecution_UnknownError)(callbacks->obj, taskParams);
            }
        }
        else
        {
            authReason = Auth::REASON_UNAUTHENTICATED;
            CALLBACK(callbacks->CB_TokenValidation_Failed)(callbacks->obj, taskParams, sJWTToken, CallbackDefinitions::TOKEN_VALIDATION_ERROR);
        }
    }

    response["txt"] = getReasonText(authReason);
    response["val"] = static_cast<Json::UInt>(authReason);
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
