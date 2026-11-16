#include "test_support.h"

namespace WebRESTFulTests {

std::vector<Case> endpointCases(const std::string &suite)
{
    std::vector<Case> cases;
    const std::vector<Method> supported = {Method::GET, Method::POST, Method::PUT, Method::DELETE, Method::PATCH};
    if (suite == "routing")
    {
        for (const auto method : supported)
        {
            cases.push_back({"dispatch." + HTTP::MethodToString(method), [method, supported]
            {
                Fixture f;
                f.add(method);
                require(f.call(method) == Result::SUCCESS && f.probe.calls == 1, "Enum dispatch failed");
                require(*f.response.responseJSON() == boost::json::object{{"ok", true}}, "Callback response was lost");
                require(f.endpoints.handleEndpoint(HTTP::MethodToString(method), "resource", f.context, {}, false, {}, &f.response)
                            == Result::SUCCESS && f.probe.calls == 2, "String dispatch failed");
                for (const auto other : supported)
                {
                    if (other != method)
                    {
                        require(f.call(other) == Result::RESOURCE_NOT_FOUND, "Method maps are not isolated");
                    }
                }
            }});
        }
        for (const auto method : {static_cast<Method>(5), static_cast<Method>(6), static_cast<Method>(255), Method::UNKNOWN})
        {
            cases.push_back({"unsupported." + std::to_string(static_cast<int>(method)), [method]
            {
                Fixture f;
                require(!f.endpoints.addEndpoint(method, "resource", Web::Security::NONE, {}, &f.probe, Probe::invoke),
                        "Unsupported method registered");
                f.error(f.call(method), Result::INVALID_METHOD_MODE, Code::S_400_BAD_REQUEST, "Invalid Method Mode");
                require(f.endpoints.handleEndpoint(method, "resource", f.context, {}, false, {}, nullptr)
                            == Result::INVALID_METHOD_MODE, "Null response changed method rejection");
            }});
        }
        for (const std::string method : {"", "get", "post", "BREW", " GET", "POST "})
        {
            cases.push_back({"invalid_method_string." + method, [method]
            {
                Fixture f;
                f.add();
                f.error(f.endpoints.handleEndpoint(method, "resource", f.context, {}, false, {}, &f.response),
                        Result::INVALID_METHOD_MODE, Code::S_400_BAD_REQUEST, "Invalid method mode string");
            }});
        }
        for (const std::string path : {"missing", "Resource", "/resource", "resource/", "resource/child", "resource?x=1"})
        {
            cases.push_back({"exact_path." + path, [path]
            {
                Fixture f;
                f.add();
                f.error(f.call(Method::POST, path), Result::RESOURCE_NOT_FOUND, Code::S_404_NOT_FOUND, "Resource not found");
            }});
        }
        cases.push_back({"replacement_and_full_definition", []
        {
            Fixture f;
            f.add();
            Probe replacement;
            replacement.reply = APIReturn(boost::json::object{{"replacement", true}});
            Web::RESTful::RESTfulAPIEndpointFullDefinition definition;
            definition.endpointDefinition = Probe::invoke;
            definition.context = &replacement;
            definition.security.set(Web::Security::NONE, {});
            require(f.endpoints.addEndpoint(Method::POST, "resource", definition), "Replacement failed");
            require(f.call() == Result::SUCCESS && replacement.calls == 1 && f.probe.calls == 0, "Old callback was retained");
            require(f.response.responseJSON()->at("replacement") == true, "Replacement response missing");
        }});
        cases.push_back({"null_callback_is_missing", []
        {
            Fixture f;
            require(f.endpoints.addEndpoint(Method::POST, "resource", {}), "Null definition registration failed");
            f.error(f.call(), Result::RESOURCE_NOT_FOUND, Code::S_404_NOT_FOUND, "Resource not found");
        }});
        cases.push_back({"null_response_does_not_invoke", []
        {
            Fixture f;
            f.add();
            require(f.endpoints.handleEndpoint(Method::POST, "resource", f.context, {}, false, {}, nullptr)
                        == Result::INTERNAL_ERROR && f.probe.calls == 0, "Null output invoked callback");
            require(f.endpoints.handleEndpoint(Method::POST, "missing", f.context, {}, false, {}, nullptr)
                        == Result::RESOURCE_NOT_FOUND, "Null output changed missing resource result");
        }});
        cases.push_back({"callback_error_headers_and_cookie_survive", []
        {
            Fixture f;
            f.add();
            f.probe.reply.setError(Code::S_409_CONFLICT, "conflict", "Already exists");
            f.probe.reply.addHeader("X-Result", "conflict");
            f.probe.reply.cookiesMap["session"].value = "abc";
            f.probe.reply.redirectURL = "/next";
            require(f.call() == Result::SUCCESS && f.response.getHTTPResponseCode() == Code::S_409_CONFLICT,
                    "Application error was treated as dispatch failure");
            require(f.response.responseJSON()->at("error") == "conflict", "Application error body lost");
            require(f.response.httpExtraHeaders.at("X-Result") == "conflict", "Response headers lost");
            require(f.response.cookiesMap.at("session").value == "abc" && f.response.redirectURL == "/next", "Cookie or redirect lost");
        }});
    }
    else if (suite == "security")
    {
        // Exhaust all four requirement combinations, four received combinations,
        // scope subsets and admin states. Authentication always precedes scope checks.
        for (unsigned requirements = 0; requirements < 4; ++requirements)
        {
            for (unsigned received = 0; received < 4; ++received)
            {
                for (unsigned scopes = 0; scopes < 4; ++scopes)
                {
                    for (bool admin : {false, true})
                    {
                        const std::string name = "matrix.required=" + std::to_string(requirements) + ".received="
                            + std::to_string(received) + ".scopes=" + std::to_string(scopes) + ".admin=" + std::to_string(admin);
                        cases.push_back({name, [requirements, received, scopes, admin]
                        {
                            Fixture f;
                            f.add(Method::POST, static_cast<Web::Security::Requirements>(requirements), {"READ", "WRITE"});
                            std::set<std::string> current = {"EXTRA"};
                            if (scopes & 1U)
                            {
                                current.insert("READ");
                            }
                            if (scopes & 2U)
                            {
                                current.insert("WRITE");
                            }
                            Web::Security::ReceivedAuth auth{bool(received & 1U), bool(received & 2U)};
                            const auto result = f.call(Method::POST, "resource", current, admin, auth);
                            if ((requirements & 1U) && !(received & 1U))
                            {
                                f.error(result, Result::AUTHENTICATION_REQUIRED, Code::S_403_FORBIDDEN, "JWT Authentication Header Required");
                            }
                            else if ((requirements & 2U) && !(received & 2U))
                            {
                                f.error(result, Result::AUTHENTICATION_REQUIRED, Code::S_403_FORBIDDEN, "JWT Authentication Cookie Required");
                            }
                            else if (!admin && scopes != 3)
                            {
                                f.error(result, Result::INVALID_SCOPE, Code::S_401_UNAUTHORIZED, "Invalid Scope");
                            }
                            else
                            {
                                require(result == Result::SUCCESS && f.probe.calls == 1, "Authorized request rejected");
                            }
                        }});
                    }
                }
            }
        }
        cases.push_back({"scope_names_are_case_sensitive", []
        {
            Fixture f;
            f.add(Method::POST, Web::Security::NONE, {"READ"});
            f.error(f.call(Method::POST, "resource", {"read"}), Result::INVALID_SCOPE, Code::S_401_UNAUTHORIZED, "Invalid Scope");
        }});
        cases.push_back({"default_full_definition_requires_both_credentials", []
        {
            Fixture f;
            Web::RESTful::RESTfulAPIEndpointFullDefinition definition;
            definition.context = &f.probe;
            definition.endpointDefinition = Probe::invoke;
            require(f.endpoints.addEndpoint(Method::POST, "resource", definition), "Registration failed");
            f.error(f.call(), Result::AUTHENTICATION_REQUIRED, Code::S_403_FORBIDDEN, "JWT Authentication Header Required");
        }});
        cases.push_back({"security_configuration_reset", []
        {
            Web::Security::Configuration config;
            config.set(Web::Security::JWT_HEADER_AUTH, {"READ"});
            require(config.requireJWTHeaderAuthentication && !config.requireJWTCookieAuthentication
                        && config.requiredScopes == std::set<std::string>{"READ"}, "Header configuration incorrect");
            config.set(Web::Security::NONE, {});
            require(!config.requireJWTHeaderAuthentication && !config.requireJWTCookieAuthentication
                        && config.requiredScopes.empty(), "Security configuration retained old requirements");
        }});
    }
    else if (suite == "input")
    {
        cases.push_back({"client_details_and_token_forwarding", []
        {
            Fixture f;
            f.add();
            JWT::Token token;
            token.setSubject("account-123");
            f.context.jwtToken = &token;
            require(f.call() == Result::SUCCESS, "Dispatch failed");
            require(f.probe.subject == "account-123", "Token was replaced");
            require(f.probe.details.ipAddress == "192.0.2.4" && f.probe.details.tlsCommonName == "test-client"
                        && f.probe.details.userAgent == "curl/8.0", "Client metadata not forwarded");
        }});
        for (const std::string body : {"{}", "[]", "null", "true", "42", "\"hello\"", "{\"nested\":[1,{\"unicode\":\"ñ\"}]}"})
        {
            cases.push_back({"json_body." + body, [body]
            {
                Fixture f;
                f.add();
                f.request.content.setContainerType(HTTP::Content::ContainerType::JSON);
                const auto stream = f.request.content.getJSONVars();
                require(stream != nullptr, "JSON content container missing");
                require(stream->writeString(body) && stream->writeEOF(), "JSON fixture failed");
                require(f.call() == Result::SUCCESS && f.probe.input == boost::json::parse(body), "JSON value changed");
            }});
        }
        cases.push_back({"get_encoded_json", []
        {
            Fixture f;
            f.add(Method::GET);
            HTTP::HTTPv1_Server parsed(std::make_shared<StreamableString>());
            feed(parsed, rawRequest("GET", "/resource?%7B%22name%22%3A%22a%2Bb%22%7D"));
            f.context.clientRequest = &parsed.clientRequest;
            require(f.call(Method::GET) == Result::SUCCESS && f.probe.input == boost::json::object{{"name", "a+b"}},
                    "GET JSON not decoded");
        }});
        for (const std::string query : {"x=1", "%7B", "%5B1%2C%5D", "%7B%22x%22%3A%7D"})
        {
            cases.push_back({"get_invalid_json." + query, [query]
            {
                Fixture f;
                f.add(Method::GET);
                HTTP::HTTPv1_Server parsed(std::make_shared<StreamableString>());
                feed(parsed, rawRequest("GET", "/resource?" + query));
                f.context.clientRequest = &parsed.clientRequest;
                f.error(f.call(Method::GET), Result::INTERNAL_ERROR, Code::S_400_BAD_REQUEST, "Bad Input JSON Parsing during GET");
            }});
        }
        cases.push_back({"empty_get_and_non_json_reset_input", []
        {
            Fixture f;
            f.add(Method::GET);
            f.add();
            f.context.emptyJSON = nullptr;
            boost::json::value stale = boost::json::object{{"stale", true}};
            f.context.inputJSON = &stale;
            f.request.requestLine.setHTTPMethod("GET");
            require(f.call(Method::GET) == Result::SUCCESS && f.probe.input.is_null(), "Empty GET retained stale input");
            f.request.requestLine.setHTTPMethod("POST");
            f.context.inputJSON = &stale;
            require(f.call() == Result::SUCCESS && f.probe.input.is_null(), "Non-JSON request retained stale input");
        }});
        for (const auto method : {Method::GET, Method::POST, Method::PUT, Method::PATCH, Method::DELETE})
        {
            cases.push_back({"explicit_input_override_and_reset." + std::to_string(static_cast<int>(method)), [method]
            {
                Fixture f;
                f.add(method);
                const std::string line = "GET /resource?name=alice HTTP/1.1\r\n";
                require(f.request.requestLine.writeIntoSubParser(line.data(), line.size()) == line.size(), "Cannot parse request line");
                boost::json::value arguments = boost::json::object{};
                f.context.explicitInputJSON = &arguments;
                require(f.call(method) == Result::SUCCESS && f.probe.input == arguments, "Explicit empty object rejected");
                arguments = nullptr;
                require(f.call(method) == Result::SUCCESS && f.probe.input.is_null(), "Explicit null fell back to URL input");
                f.context.explicitInputJSON = nullptr;
                f.probe.calls = 0;
                f.error(f.call(method), Result::INTERNAL_ERROR, Code::S_400_BAD_REQUEST, "Bad Input JSON Parsing during GET");
            }});
        }
        for (const std::string body : {"{\"body\":true}", ""})
        {
            cases.push_back({"explicit_input_overrides_post_body." + body, [body]
            {
                Fixture f;
                f.add();
                f.request.content.setContainerType(HTTP::Content::ContainerType::JSON);
                const auto stream = f.request.content.getJSONVars();
                require(stream != nullptr, "JSON content container missing");
                if (!body.empty())
                {
                    require(stream->writeString(body) && stream->writeEOF(), "Cannot write JSON body");
                }
                const auto originalInput = *stream->getValue();
                boost::json::value arguments = boost::json::object{{"tag", 42}};
                f.context.explicitInputJSON = &arguments;
                require(f.call() == Result::SUCCESS && f.probe.input == arguments, "HTTP body replaced explicit arguments");
                f.context.explicitInputJSON = nullptr;
                f.probe.calls = 0;
                require(f.call() == Result::SUCCESS && f.probe.input == originalInput
                            && f.context.inputJSON == stream->getValue(), "HTTP body input was not restored");
            }});
        }
        cases.push_back({"explicit_input_preserves_security_checks", []
        {
            Fixture f;
            f.add(Method::GET, Web::Security::JWT_HEADER_AUTH, {"READ"});
            boost::json::value arguments = boost::json::object{};
            f.context.explicitInputJSON = &arguments;
            f.error(f.call(Method::GET), Result::AUTHENTICATION_REQUIRED, Code::S_403_FORBIDDEN, "JWT Authentication Header Required");
            const Web::Security::ReceivedAuth auth{true, false};
            f.error(f.call(Method::GET, "resource", {}, false, auth), Result::INVALID_SCOPE, Code::S_401_UNAUTHORIZED, "Invalid Scope");
            require(f.call(Method::GET, "resource", {"READ"}, false, auth) == Result::SUCCESS
                        && f.probe.calls == 1 && f.probe.input == arguments, "Authorized embedded call failed");
        }});
    }
    return cases;
}

} // namespace WebRESTFulTests
