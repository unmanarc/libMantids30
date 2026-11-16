#include "test_support.h"

#include <ctime>

namespace WebRESTFulTests {
namespace {

struct Application
{
    Engine engine;
    Probe probe;
    std::shared_ptr<Endpoints> endpoints = std::make_shared<Endpoints>();

    Application()
    {
        engine.config.appName = "test-app";
        engine.config.permittedAPIOrigins = {"https://app.test"};
        engine.config.permittedLoginOrigins = {"https://iam.test"};
        engine.config.jwtValidator = std::make_shared<JWT>();
        engine.config.jwtValidator->setSharedSecret("webrestful-integration-test-secret");
        engine.config.jwtSigner = engine.config.jwtValidator;
        engine.endpointsHandler[1] = endpoints;
        for (const auto method : {Method::GET, Method::POST, Method::PUT, Method::PATCH, Method::DELETE})
        {
            add("echo", Web::Security::NONE, {}, method);
        }
        add("header", Web::Security::JWT_HEADER_AUTH, {"READ"});
        add("cookie", Web::Security::JWT_COOKIE_AUTH, {"READ"});
        add("callback", Web::Security::NONE);
    }

    void add(const std::string &path, Web::Security::Requirements security, const std::set<std::string> &scopes = {}, Method method = Method::POST)
    {
        require(endpoints->addEndpoint(method, path, security, scopes, &probe, Probe::invoke), "Cannot register application endpoint");
    }

    std::string token(const std::string &app = "test-app", const std::string &type = "access", bool scope = true,
                      bool admin = false, bool expired = false)
    {
        JWT::Token token;
        token.setSubject("account-123");
        token.setDomain("example.test");
        token.setJwtId("session-456");
        token.setClaim("app", app);
        token.setClaim("type", type);
        token.setClaim("isAdmin", admin);
        token.setExpirationTime(std::time(nullptr) + (expired ? -3600 : 3600));
        token.setIssuedAt(std::time(nullptr) - 7200);
        token.setNotBefore(std::time(nullptr) - 7200);
        if (scope)
        {
            token.addScope("READ");
        }
        return engine.config.jwtSigner->signFromToken(token, false);
    }

    Reply request(const std::string &path, const std::string &headers = "", const std::string &body = "{}", size_t fragment = 0)
    {
        return sendRequest(engine, rawRequest("POST", path, body, headers), fragment);
    }

    void error(const Reply &reply, unsigned status, const std::string &error)
    {
        require(reply.status == status, "Expected status " + std::to_string(status) + ", got " + std::to_string(reply.status));
        require(boost::json::parse(reply.body).at("error") == boost::json::value(error), "Wrong wire error body");
        require(probe.calls == 0, "Rejected wire request reached endpoint");
    }
};

} // namespace

std::vector<Case> integrationCases()
{
    std::vector<Case> cases;
    for (size_t fragment : {size_t(0), size_t(1), size_t(7)})
    {
        for (const std::string method : {"POST", "PUT", "PATCH", "DELETE", "GET"})
        {
            cases.push_back({"http_json." + method + ".fragment=" + std::to_string(fragment), [method, fragment]
            {
                Application app;
                const std::string body = R"({"nested":[true,null,42],"text":"a+b"})";
                const std::string path = method == "GET" ? "/api/v1/echo?%7B%22id%22%3A42%7D" : "/api/v1/echo";
                const auto reply = sendRequest(app.engine, rawRequest(method, path, method == "GET" ? "" : body), fragment);
                require(reply.status == 200 && boost::json::parse(reply.body) == boost::json::object{{"ok", true}}, "JSON wire response incorrect");
                require(app.probe.calls == 1 && app.probe.input == (method == "GET" ? boost::json::value(boost::json::object{{"id", 42}})
                            : boost::json::parse(body)), "HTTP request JSON did not reach endpoint once");
                require(app.probe.details.ipAddress == "192.0.2.4" && app.probe.details.userAgent == "curl/8.0", "Wire metadata lost");
            }});
        }
    }
    for (const std::string path : {"/api/v99/echo", "/api/v1/missing", "/api/v1/Echo"})
    {
        cases.push_back({"missing_resource." + path, [path]
        {
            Application app;
            const auto reply = app.request(path);
            app.error(reply, 404, path == "/api/v99/echo" ? "invalid_api_request" : "invalid_invokation");
        }});
    }
    for (const std::string path : {"header", "cookie"})
    {
        cases.push_back({"authentication_required." + path, [path]
        {
            Application app;
            app.error(app.request("/api/v1/" + path), 403, "invalid_invokation");
        }});
        cases.push_back({"valid_jwt." + path, [path]
        {
            Application app;
            const auto jwt = app.token();
            const std::string headers = path == "header" ? "Authorization: Bearer " + jwt + "\r\n" : "Cookie: AccessToken=" + jwt + "\r\n";
            const auto reply = app.request("/api/v1/" + path, headers);
            require(reply.status == 200 && app.probe.calls == 1 && app.probe.subject == "account-123", "Valid JWT rejected or lost identity");
        }});
        cases.push_back({"wrong_credential_channel." + path, [path]
        {
            Application app;
            const auto jwt = app.token();
            const std::string headers = path == "header" ? "Cookie: AccessToken=" + jwt + "\r\n" : "Authorization: Bearer " + jwt + "\r\n";
            app.error(app.request("/api/v1/" + path, headers), 403, "invalid_invokation");
        }});
    }
    for (const std::string variant : {"malformed", "signature", "expired", "wrong_app", "refresh", "no_validator", "missing_scope", "admin"})
    {
        cases.push_back({"jwt_validation." + variant, [variant]
        {
            Application app;
            std::string jwt = app.token(variant == "wrong_app" ? "another-app" : "test-app",
                                        variant == "refresh" ? "refresh" : "access",
                                        variant != "missing_scope" && variant != "admin", variant == "admin", variant == "expired");
            if (variant == "malformed")
            {
                jwt = "invalid.jwt";
            }
            else if (variant == "signature")
            {
                JWT other;
                other.setSharedSecret("wrong-secret");
                jwt = other.sign(boost::json::object{{"sub", "account-123"}, {"app", "test-app"}, {"type", "access"},
                                                     {"exp", std::time(nullptr) + 3600}});
            }
            else if (variant == "no_validator")
            {
                app.engine.config.jwtValidator.reset();
            }
            const auto reply = app.request("/api/v1/header", "Authorization: Bearer " + jwt + "\r\n");
            if (variant == "admin")
            {
                require(reply.status == 200 && app.probe.calls == 1, "Admin scope bypass failed");
            }
            else
            {
                app.error(reply, variant == "missing_scope" ? 401 : 403, "invalid_invokation");
            }
        }});
    }
    cases.push_back({"header_precedence_over_cookie", []
    {
        Application app;
        const auto jwt = app.token();
        app.error(app.request("/api/v1/cookie", "Authorization: Bearer invalid\r\nCookie: AccessToken=" + jwt + "\r\n"), 403, "invalid_invokation");
        const auto reply = app.request("/api/v1/header", "Authorization: Bearer " + jwt + "\r\nCookie: AccessToken=invalid\r\n");
        require(reply.status == 200 && app.probe.calls == 1, "Valid header did not take precedence");
    }});
    for (const std::string channel : {"header", "cookie"})
    {
        cases.push_back({"keep_alive_drops_authentication." + channel, [channel]
        {
            Application app;
            const auto jwt = app.token();
            const std::string auth = channel == "header" ? "Authorization: Bearer " + jwt + "\r\n"
                : "Cookie: AccessToken=" + jwt + "\r\n";
            auto first = rawRequest("POST", "/api/v1/" + channel, "{}", auth);
            first.replace(first.find("Connection: close"), 17, "Connection: keep-alive");
            const std::string second = rawRequest("POST", "/api/v1/" + channel, "{}");
            auto socket = std::make_shared<Mantids30::Network::Sockets::Socket_Stream_Dummy>();
            auto handler = app.engine.createNewAPIServer_ClientHandler(&app.engine, socket);
            handler->config = &app.engine.config;
            app.engine.config.webLog = std::make_shared<Mantids30::Runtime::Logs::WebLog>();
            app.engine.config.webLog->config.useThreadedQueue = false;
            feed(*handler, first + second, 7);
            const auto raw = socket->getReceiver()->toString().value();
            require(app.probe.calls == 1 && raw.find("HTTP/1.1 403") != std::string::npos,
                    "Unauthenticated keep-alive request reused previous JWT authentication");
        }});
    }
    cases.push_back({"custom_cookie_name", []
    {
        Application app;
        app.engine.jwtAccessTokenName = "AppToken";
        const auto jwt = app.token();
        app.error(app.request("/api/v1/cookie", "Cookie: AccessToken=" + jwt + "\r\n"), 403, "invalid_invokation");
        require(app.request("/api/v1/cookie", "Cookie: AppToken=" + jwt + "\r\n").status == 200, "Custom cookie name ignored");
    }});
    for (const std::string origin : {"", "https://app.test", "https://evil.test", "https://app.test.evil"})
    {
        cases.push_back({"origin." + origin, [origin]
        {
            Application app;
            const auto reply = app.request("/api/v1/echo", origin.empty() ? "" : "Origin: " + origin + "\r\n");
            if (origin.empty() || origin == "https://app.test")
            {
                require(reply.status == 200 && app.probe.calls == 1, "Allowed origin rejected");
            }
            else
            {
                app.error(reply, 401, "invalid_security_context");
            }
        }});
    }
    for (const std::string origin : {"", "https://iam.test", "https://app.test"})
    {
        cases.push_back({"callback_origin." + origin, [origin]
        {
            Application app;
            const auto reply = app.request("/api/v1/callback", origin.empty() ? "" : "Origin: " + origin + "\r\n");
            if (origin == "https://iam.test")
            {
                require(reply.status == 200 && app.probe.calls == 1, "Allowed login origin rejected");
            }
            else
            {
                app.error(reply, 401, "invalid_security_context");
            }
        }});
    }
    cases.push_back({"dynamic_origin_receives_api_key", []
    {
        Application app;
        app.engine.config.dynamicOriginValidator = [](const std::string &origin, const std::string &key, const std::set<std::string> &) -> bool
        {
            return origin == "https://tenant.test" && key == "tenant-key";
        };
        app.error(app.request("/api/v1/echo", "Origin: https://tenant.test\r\nX-API-Key: wrong\r\n"), 401, "invalid_security_context");
        require(app.request("/api/v1/echo", "Origin: https://tenant.test\r\nX-API-Key: tenant-key\r\n").status == 200, "Dynamic origin validator failed");
    }});
    cases.push_back({"dynamic_token_validator", []
    {
        Application app;
        app.engine.config.dynamicTokenValidator = [](const std::string &token, std::string_view key, JWT::Token *output) -> bool
        {
            if (token != "tenant-token" || key != "tenant-key")
            {
                return false;
            }
            output->setSubject("tenant-account");
            output->addScope("READ");
            return true;
        };
        app.error(app.request("/api/v1/header", "Authorization: Bearer tenant-token\r\nX-API-Key: wrong\r\n"), 403, "invalid_invokation");
        const auto reply = app.request("/api/v1/header", "Authorization: Bearer tenant-token\r\nX-API-Key: tenant-key\r\n");
        require(reply.status == 200 && app.probe.subject == "tenant-account", "Dynamic token validation not used");
    }});
    cases.push_back({"options_override_and_unknown_version", []
    {
        Application app;
        Web::OptionsHandlerConfig global, specific;
        global.setAllowedOrigins({"https://app.test"}).setAllowedMethods({"GET"});
        specific.setAllowedOrigins({"https://app.test"}).setAllowedMethods({"POST", "OPTIONS"}).setAllowCredentials(true);
        app.endpoints->setEndpointOptions(global);
        app.endpoints->setEndpointOptions("echo", specific);
        const auto reply = sendRequest(app.engine, rawRequest("OPTIONS", "/api/v1/echo", "", "Origin: https://app.test\r\n"));
        require(reply.status == 204 && reply.body.empty() && app.probe.calls == 0, "Preflight invoked application or returned body");
        require(reply.raw.find("POST, OPTIONS") == std::string::npos, "Methods unexpectedly reordered");
        require(reply.raw.find("OPTIONS, POST") != std::string::npos && reply.raw.find("https://app.test") != std::string::npos,
                "Endpoint CORS configuration missing from wire response");
        const auto fallback = sendRequest(app.engine, rawRequest("OPTIONS", "/api/v1/other"));
        require(fallback.status == 204 && fallback.raw.find("GET") != std::string::npos, "Global CORS fallback failed");
        const auto missing = sendRequest(app.engine, rawRequest("OPTIONS", "/api/v99/echo"));
        require(missing.status == 404 && app.probe.calls == 0, "Unknown OPTIONS version accepted");
    }});
    cases.push_back({"application_response_on_wire", []
    {
        Application app;
        app.probe.reply.setError(Code::S_409_CONFLICT, "duplicate", "Already exists");
        app.probe.reply.addHeader("X-Application", "test");
        app.probe.reply.cookiesMap["result"].value = "created";
        const auto reply = app.request("/api/v1/echo");
        require(reply.status == 409 && boost::json::parse(reply.body).at("error") == "duplicate", "Application error response changed");
        require(reply.raw.find("X-Application: test") != std::string::npos
                    && reply.raw.find("result=created") != std::string::npos, "Application headers/cookies missing");
    }});
    cases.push_back({"alternate_api_base_and_version", []
    {
        Application app;
        app.engine.config.APIURLs = {"/service"};
        app.engine.endpointsHandler[2] = app.endpoints;
        require(app.request("/service/v2/echo").status == 200 && app.probe.calls == 1, "Alternate base/version failed");
    }});
    cases.push_back({"malformed_json_never_reaches_callback", []
    {
        Application app;
        auto socket = std::make_shared<Mantids30::Network::Sockets::Socket_Stream_Dummy>();
        auto handler = app.engine.createNewAPIServer_ClientHandler(&app.engine, socket);
        handler->config = &app.engine.config;
        const std::string raw = rawRequest("POST", "/api/v1/echo", "{\"unterminated\":");
        const auto written = handler->write(raw.data(), raw.size());
        if (written && !handler->writeStatus.finished)
        {
            (void)handler->writeEOF();
        }
        using Parser = Mantids30::Core::Memory::Streams::Parser;
        require((handler->getParseResult() == Parser::ParseResult::ERR_PARSING
                    || handler->getParseResult() == Parser::ParseResult::ERR_STREAM_WRITE) && app.probe.calls == 0,
                "Malformed JSON was not rejected before application dispatch");
    }});
    cases.push_back({"browser_upgrade_check", []
    {
        Application app;
        auto raw = rawRequest("POST", "/api/v1/echo", "{}");
        raw.replace(raw.find("curl/8.0"), 8, "Firefox/59.0");
        const auto reply = sendRequest(app.engine, raw);
        require(reply.status == 426 && app.probe.calls == 0, "Unsupported browser reached endpoint");
    }});
    cases.push_back({"initial_checks_prevent_dispatch", []
    {
        Application app;
        app.engine.config.dynamicInitialChecks = [](HTTP::HTTPv1_Base::Request *, HTTP::HTTPv1_Base::Response *) -> Code
        {
            return Code::S_403_FORBIDDEN;
        };
        const auto reply = app.request("/api/v1/echo");
        require(reply.status == 403 && app.probe.calls == 0, "Initial check ignored");
    }});
    return cases;
}

} // namespace WebRESTFulTests
