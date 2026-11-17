#include "test_support.h"

#include <Mantids30/Server/WebRESTFul/apiproxy_config.h>
#include <boost/property_tree/info_parser.hpp>
#include <sstream>
#include <future>
#include <ctime>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace WebRESTFulTests {
namespace {

class TemplatePage
{
public:
    explicit TemplatePage(const std::string &content)
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        directory = std::filesystem::temp_directory_path() / ("mantids-jfunc-" + std::to_string(stamp));
        require(std::filesystem::create_directory(directory), "Cannot create template directory");
        std::ofstream output(directory / "page.html");
        output << content;
        require(bool(output), "Cannot write template page");
    }
    ~TemplatePage()
    {
        std::error_code error;
        std::filesystem::remove_all(directory, error);
    }
    std::filesystem::path directory;
};

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
    const auto routeHandler = +[](const std::string &path, HTTP::HTTPv1_Base::Request *, HTTP::HTTPv1_Base::Response *response,
                                  const std::shared_ptr<void> &object, const Web::Sessions::SessionInfo *) -> Code
    {
        auto body = std::make_shared<StreamableString>();
        body->setValue(*std::static_pointer_cast<std::string>(object) + path);
        response->setContentDataStreamer(body);
        return Code::S_200_OK;
    };
    cases.push_back({"dynamic_routes.config_order_and_duplicate_paths", [routeHandler]
    {
        Application app;
        std::istringstream input(R"CONFIG(Proxies
{
    /z { RemoteHost first MatchExpression "IS_EQUAL($request.get.target,\"first\")" }
    /z { RemoteHost fallback }
    /a { RemoteHost last }
}
)CONFIG");
        boost::property_tree::ptree configuration;
        boost::property_tree::read_info(input, configuration);
        Mantids30::Runtime::Logs::AppLog log;
        for (const auto &entry : configuration.get_child("Proxies"))
        {
            auto params = Web::APIProxyConfig::createAPIProxyParams(&log, entry.second);
            require(bool(params), "Cannot parse configured proxy");
            app.engine.config.dynamicRequestHandlersByRoute.emplace_back(entry.first,
                Web::APIServerConfig::DynamicRequestHandlerDef{routeHandler, std::make_shared<std::string>(params->remoteHost), params->matchExpression});
        }
        require(app.engine.config.dynamicRequestHandlersByRoute[0].first == "/z", "Configuration order lost");
        require(app.request("/z/page?target=first").body == "first/page", "First duplicate rule not selected");
        require(app.request("/z/page?target=other").body == "fallback/page", "Duplicate fallback not selected");
        app.engine.config.dynamicRequestHandlersByRoute.emplace_back("/z/nested",
            Web::APIServerConfig::DynamicRequestHandlerDef{routeHandler, std::make_shared<std::string>("nested")});
        require(app.request("/z/nested/page").body == "fallback/nested/page", "Overlapping routes ignored insertion priority");
        require(app.request("/z").body == "fallback/", "Exact mount did not map to upstream root");
        app.engine.config.dynamicRequestHandlersByRoute.emplace_back("/",
            Web::APIServerConfig::DynamicRequestHandlerDef{routeHandler, std::make_shared<std::string>("root")});
        require(app.request("/zebra").body == "root/zebra", "Route matched without a path boundary");
        require(app.request("/").body == "root/", "Root mount did not match");
    }});
    for (const std::string expression : {
             R"(IS_EQUAL($session.user,"account-123"))",
             R"(IS_EQUAL($session.roles,"operator"))",
             R"(IS_EQUAL($session.scopes,"READ"))",
             R"(IS_EQUAL($session.isActive,"true") && IS_EQUAL($session.claims.app,"test-app"))"})
    {
        cases.push_back({"dynamic_routes.validated_identity." + expression, [routeHandler, expression]
        {
            Application app;
            auto condition = std::make_shared<Web::RequestMatchExpression>(expression);
            app.engine.config.dynamicRequestHandlersByRoute.emplace_back("/site",
                Web::APIServerConfig::DynamicRequestHandlerDef{routeHandler, std::make_shared<std::string>("private"), condition});
            app.engine.config.dynamicRequestHandlersByRoute.emplace_back("/site",
                Web::APIServerConfig::DynamicRequestHandlerDef{routeHandler, std::make_shared<std::string>("public")});
            JWT::Token token;
            token.setSubject("account-123");
            token.setClaim("app", "test-app");
            token.setClaim("type", "access");
            token.setExpirationTime(std::time(nullptr) + 3600);
            token.addRole("operator");
            token.addScope("READ");
            const auto signedToken = app.engine.config.jwtSigner->signFromToken(token, false);
            require(app.request("/site/page", "Authorization: Bearer " + signedToken + "\r\n").body == "private/page", "Verified identity did not match");
            require(app.request("/site/page", "Cookie: AccessToken=" + signedToken + "\r\n").body == "private/page", "Cookie identity did not match");
            require(app.request("/site/page").body == "public/page", "Anonymous request matched authenticated rule");
            require(app.request("/site/page", "Authorization: Bearer invalid\r\n").body == "public/page", "Invalid token matched authenticated rule");
        }});
    }
    cases.push_back({"dynamic_routes.request_context_and_no_match", [routeHandler]
    {
        Application app;
        auto condition = std::make_shared<Web::RequestMatchExpression>(
            R"(IS_EQUAL($client.ip,"192.0.2.4") && IS_EQUAL($request.method,"POST") && IS_EQUAL($request.uri,"/site/page") && IS_EQUAL($request.get.tag,"two") && IS_EQUAL($request.headers.X-TARGET,"blue") && IS_EQUAL($request.headers.X-MULTI,"second"))");
        app.engine.config.dynamicRequestHandlersByRoute.emplace_back("/site",
            Web::APIServerConfig::DynamicRequestHandlerDef{routeHandler, std::make_shared<std::string>("selected"), condition});
        require(app.request("/site/page?tag=one&tag=two", "X-Target: blue\r\nX-Multi: first\r\nX-Multi: second\r\n").body == "selected/page", "Request context lost fields or repeated values");
        require(app.request("/site/page?tag=two", "X-Target: red\r\n").status == 404, "No match did not fall through to files");
    }});
    cases.push_back({"dynamic_routes.json_and_form_body", [routeHandler]
    {
        Application app;
        app.engine.config.dynamicRequestHandlersByRoute.emplace_back("/site",
            Web::APIServerConfig::DynamicRequestHandlerDef{routeHandler, std::make_shared<std::string>("json"),
                std::make_shared<Web::RequestMatchExpression>(R"(IS_EQUAL($request.json.target,"blue") && IS_EQUAL($request.json.version,2) && IS_EQUAL($request.json.flags,"true"))")});
        app.engine.config.dynamicRequestHandlersByRoute.emplace_back("/site",
            Web::APIServerConfig::DynamicRequestHandlerDef{routeHandler, std::make_shared<std::string>("form"),
                std::make_shared<Web::RequestMatchExpression>(R"(IS_EQUAL($request.post.target,"blue"))")});
        require(app.request("/site/page", "", R"({"target":"blue","version":2,"flags":[false,true]})").body == "json/page", "JSON body not available");
        require(sendRequest(app.engine, rawRequest("POST", "/site/page", "target=blue", "", "application/x-www-form-urlencoded")).body == "form/page", "Form body not available");
    }});
    cases.push_back({"dynamic_routes.evaluation_error_does_not_fall_through", [routeHandler]
    {
        Application app;
        app.engine.config.dynamicRequestHandlersByRoute.emplace_back("/site",
            Web::APIServerConfig::DynamicRequestHandlerDef{routeHandler, std::make_shared<std::string>("private"),
                std::make_shared<Web::RequestMatchExpression>(R"(IS_EQUAL($request.method,"POST") && REGEX_MATCH($client.ip,"["))")});
        app.engine.config.dynamicRequestHandlersByRoute.emplace_back("/site",
            Web::APIServerConfig::DynamicRequestHandlerDef{routeHandler, std::make_shared<std::string>("fallback")});
        require(app.request("/site/page").status == 500, "Evaluation error selected a broader rule");
    }});
    cases.push_back({"dynamic_routes.invalid_configuration_and_concurrent_evaluation", []
    {
        Mantids30::Runtime::Logs::AppLog log;
        boost::property_tree::ptree configuration;
        configuration.put("MatchExpression", "invalid expression");
        require(!Web::APIProxyConfig::createAPIProxyParams(&log, configuration), "Invalid expression accepted");
        Web::RequestMatchExpression invalid("invalid expression");
        bool matched = true;
        std::string error;
        require(!invalid.isCompiled() && !invalid.getLastCompilerError().empty(), "Compilation failure was not reported");
        require(!invalid.evaluate(boost::json::object{}, matched, &error) && !matched && !error.empty(), "Evaluation failure was not reported");
        configuration.put("MatchExpression", "");
        require(!Web::APIProxyConfig::createAPIProxyParams(&log, configuration), "Explicit empty expression accepted");
        configuration.erase("MatchExpression");
        configuration.put("UseTLS", "invalid");
        require(!Web::APIProxyConfig::createAPIProxyParams(&log, configuration), "Invalid boolean accepted");
        configuration.put("UseTLS", "true");
        configuration.put("RemotePort", "invalid");
        require(!Web::APIProxyConfig::createAPIProxyParams(&log, configuration), "Invalid port accepted");
        auto condition = std::make_shared<Web::RequestMatchExpression>(R"(REGEX_MATCH($client.ip,"192[.]0[.]2[.][0-9]+"))");
        std::vector<std::future<void>> workers;
        for (size_t index = 0; index < 8; ++index)
        {
            workers.emplace_back(std::async(std::launch::async, [condition]
            {
                for (size_t iteration = 0; iteration < 100; ++iteration)
                {
                    bool matched = false;
                    require(condition->evaluate(boost::json::object{{"client", boost::json::object{{"ip", "192.0.2.4"}}}}, matched) && matched,
                            "Concurrent evaluation failed");
                }
            }));
        }
        for (auto &worker : workers)
        {
            worker.get();
        }
    }});
    for (const std::string pageMethod : {"GET", "POST"})
    {
        for (const std::string query : {"", "?name=alice&submit=1", "?%7B%22url%22%3Atrue%7D", "?%7B"})
        {
            for (size_t fragment : {size_t(0), size_t(1), size_t(7)})
            {
                cases.push_back({"embedded_jfunc_http." + pageMethod + query + ".fragment=" + std::to_string(fragment),
                                 [pageMethod, query, fragment]
                {
                    Application app;
                    TemplatePage page(
                        "<!--<%jfunc/empty:GET/v1/echo({})%>-->\n"
                        "<!--<%jfunc/value:GET/v1/echo({\"tag\":42})%>-->\n"
                        "<!--<%jfunc/last:GET/v1/echo(null)%>-->");
                    require(app.engine.config.setDocumentRootPath(page.directory.string()), "Cannot set template document root");
                    using Filter = Web::ResourcesFilter;
                    app.engine.config.resourceFilter = std::make_shared<Filter>();
                    Filter::Filter rule;
                    rule.uriRegexs = {"/page.html"};
                    Filter::Action process;
                    process.type = Filter::ActionType::PROCESS_AS;
                    process.processingMode = Filter::ProcessingMode::HTMLIENGINE;
                    Filter::Action accept;
                    accept.type = Filter::ActionType::ACCEPT;
                    rule.actions = {process, accept};
                    app.engine.config.resourceFilter->addFilter(rule);
                    struct Inputs
                    {
                        std::vector<boost::json::value> values;
                        static APIReturn invoke(void *context, const RequestContext &request, Web::Sessions::ClientDetails &)
                        {
                            auto &inputs = *static_cast<Inputs *>(context);
                            inputs.values.push_back(*request.inputJSON);
                            return APIReturn(*request.inputJSON);
                        }
                    } inputs;
                    require(app.endpoints->addEndpoint(Method::GET, "echo", Web::Security::NONE, {}, &inputs, Inputs::invoke),
                            "Cannot register embedded echo");
                    const std::string body = pageMethod == "POST" ? "tag=body&submit=1" : "";
                    const auto reply = sendRequest(app.engine, rawRequest(pageMethod, "/page.html" + query, body, "",
                                                   "application/x-www-form-urlencoded"), fragment);
                    require(reply.status == 200, "Page request failed with status " + std::to_string(reply.status));
                    require(inputs.values == std::vector<boost::json::value>{boost::json::object{}, boost::json::object{{"tag", 42}}, nullptr},
                            "Sequential JFUNC calls inherited URL, body, or previous tag input");
                    require(reply.body.find("const empty = ") != std::string::npos
                                && reply.body.find("const value = ") != std::string::npos
                                && reply.body.find("const last = ") != std::string::npos
                                && reply.body.find("invalid_invokation") == std::string::npos,
                            "Rendered HTTP response lost JFUNC results");
                    const auto direct = sendRequest(app.engine, rawRequest("GET", "/api/v1/echo?tag=form"), fragment);
                    require(direct.status == 400 && inputs.values.size() == 3, "Direct HTTP GET accepted form parameters");
                    const auto valid = sendRequest(app.engine, rawRequest("GET", "/api/v1/echo?%7B%22direct%22%3Atrue%7D"), fragment);
                    require(valid.status == 200 && inputs.values.size() == 4
                                && inputs.values.back() == boost::json::object{{"direct", true}}, "Direct HTTP GET lost JSON input");
                }});
            }
        }
    }
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
