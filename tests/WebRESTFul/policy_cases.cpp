#include "test_support.h"
#include <Mantids30/Server/WebRESTFul/clienthandler.h>
#include <Mantids30/Server/WebRESTFul/htmliengine.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <unistd.h>

namespace WebRESTFulTests {
namespace {

class FilterFile
{
public:
    explicit FilterFile(const std::string &text)
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        directory = std::filesystem::temp_directory_path() / ("mantids-webrestful-" + std::to_string(stamp));
        require(std::filesystem::create_directory(directory), "Cannot create fixture directory");
        path = directory / "resources.conf";
        std::ofstream output(path);
        output << text;
        require(bool(output), "Cannot write filter fixture");
    }
    ~FilterFile()
    {
        std::error_code error;
        std::filesystem::remove_all(directory, error);
    }
    std::filesystem::path path;
    std::filesystem::path directory;
};

using Filter = Web::ResourcesFilter;
using Action = Filter::Action;
using ActionType = Filter::ActionType;

class LoggingHandler : public Web::ClientHandler
{
public:
    using Web::ClientHandler::ClientHandler;
    using Web::APIServer_ClientHandler::handleAPIRequestFromFile;
    using Web::APIServer_ClientHandler::log;
    APIReturn handleAPIRequest(const std::string &base, const uint32_t &version, const std::string &method,
                               const std::string &endpoint, const boost::json::value &parameters) override
    {
        if (endpoint == "throw")
        {
            log(Mantids30::Runtime::Logs::LogLevel::WARNING, "test", 2048, "inner failure");
            throw std::runtime_error("API failure");
        }
        if (endpoint == "nested")
        {
            try
            {
                handleAPIRequestFromFile("/inner.html", base, version, method, "throw", parameters);
            }
            catch (const std::runtime_error &)
            {
                log(Mantids30::Runtime::Logs::LogLevel::WARNING, "test", 2048, "outer restored");
            }
        }
        return Web::ClientHandler::handleAPIRequest(base, version, method, endpoint, parameters);
    }
};

std::string captureWarnings(const std::function<void()> &action)
{
    const auto closeFile = [](FILE *file) { std::fclose(file); };
    std::unique_ptr<FILE, decltype(closeFile)> output(std::tmpfile(), closeFile);
    require(bool(output), "Cannot create log capture");
    std::fflush(stdout);
    const int saved = ::dup(STDOUT_FILENO);
    require(saved >= 0, "Cannot save stdout");
    struct RestoreOutput
    {
        int saved;
        ~RestoreOutput()
        {
            std::fflush(stdout);
            ::dup2(saved, STDOUT_FILENO);
            ::close(saved);
        }
    } restore{saved};
    require(::dup2(::fileno(output.get()), STDOUT_FILENO) >= 0, "Cannot redirect stdout");
    action();
    std::fflush(stdout);
    std::rewind(output.get());
    std::string result;
    char buffer[1024];
    while (const auto count = std::fread(buffer, 1, sizeof(buffer), output.get()))
    {
        result.append(buffer, count);
    }
    return result;
}

Filter::Filter denyFilter()
{
    Filter::Filter rule;
    rule.uriRegexs = {"/private/.*", "/settings"};
    Action deny;
    deny.type = ActionType::DENY;
    deny.statusCode = 403;
    rule.actions = {deny};
    return rule;
}

} // namespace

std::vector<Case> policyCases(const std::string &suite)
{
    std::vector<Case> cases;
    if (suite == "cors")
    {
        cases.push_back({"defaults", []
        {
            Web::OptionsHandlerConfig config;
            Endpoints endpoints;
            require(!endpoints.isOptionsEnabled(), "OPTIONS enabled by default");
            const auto response = endpoints.buildCORSOptionsResponse(config);
            require(response.getHTTPResponseCode() == Code::S_204_NO_CONTENT, "Wrong preflight status");
            const auto &headers = response.httpExtraHeaders;
            require(headers.at("Access-Control-Allow-Methods") == "GET, POST, PUT, DELETE, OPTIONS", "Wrong default methods");
            require(headers.at("Access-Control-Allow-Headers") == "Content-Type, Authorization", "Wrong default headers");
            require(headers.at("Access-Control-Max-Age") == "86400", "Wrong default max age");
            require(headers.count("Access-Control-Allow-Origin") == 0 && headers.count("Access-Control-Allow-Credentials") == 0
                        && headers.count("Access-Control-Expose-Headers") == 0, "Unexpected optional header");
        }});
        cases.push_back({"fluent_setters_and_deduplication", []
        {
            Web::OptionsHandlerConfig config;
            auto &same = config.setAllowedOrigins({"https://b.test"}).insertAllowedOrigin("https://a.test")
                .setAllowedMethods({"POST"}).insertAllowedMethod("GET").insertAllowedMethod("GET")
                .setAllowedHeaders({"Content-Type"}).insertAllowedHeader("Authorization")
                .setExposeHeaders({"X-Z"}).insertExposeHeader("X-A").setAllowCredentials(true).setMaxAgeSeconds(120);
            require(&same == &config, "Fluent setter returned another object");
            require(config.getMethodsHeader() == "GET, POST" && config.getHeadersHeader() == "Authorization, Content-Type"
                        && config.getExposeHeadersHeader() == "X-A, X-Z", "Header lists are not sorted/deduplicated");
            require(!config.allowsAllOrigins() && config.allowsOrigin("https://a.test") && !config.allowsOrigin("https://evil.test"),
                    "Origin allowlist incorrect");
            require(!config.allowsOrigin("https://a.test.evil") && !config.allowsOrigin("https://A.test"), "Origin matching is not exact");
            APIReturn response;
            config.configureAPIReturnOptionsHeaders(response, "https://b.test");
            require(response.httpExtraHeaders.at("Access-Control-Allow-Origin") == "https://b.test", "Allowed origin not reflected");
            require(response.httpExtraHeaders.at("Access-Control-Allow-Credentials") == "true"
                        && response.httpExtraHeaders.at("Access-Control-Max-Age") == "120", "Credentials or max age missing");
            require(response.httpExtraHeaders.at("Access-Control-Expose-Headers") == "X-A, X-Z", "Exposed headers missing");
            config.setAllowedMethods({"PATCH"}).setAllowedHeaders({"X-Key"}).setExposeHeaders({}).setAllowedOrigins({"*"});
            require(config.getMethodsHeader() == "PATCH" && config.getHeadersHeader() == "X-Key"
                        && config.getExposeHeadersHeader().empty(), "Setters appended instead of replacing");
        }});
        for (const std::string origin : {"", "https://app.test", "null"})
        {
            cases.push_back({"wildcard." + origin, [origin]
            {
                Web::OptionsHandlerConfig config;
                config.insertAllowedOrigin("*");
                APIReturn response;
                config.configureAPIReturnOptionsHeaders(response, origin);
                require(config.allowsAllOrigins() && config.allowsOrigin(origin) && config.getOriginsHeader() == "*",
                        "Wildcard matching failed");
                require(response.httpExtraHeaders.at("Access-Control-Allow-Origin") == "*", "Wildcard header missing");
            }});
        }
        for (int age : {0, -1, 1, 3600})
        {
            cases.push_back({"max_age." + std::to_string(age), [age]
            {
                Web::OptionsHandlerConfig config;
                config.setMaxAgeSeconds(age);
                APIReturn response;
                config.configureAPIReturnOptionsHeaders(response, "");
                require(response.httpExtraHeaders.count("Access-Control-Max-Age") == (age > 0 ? 1U : 0U), "Wrong max-age presence");
            }});
        }
        cases.push_back({"endpoint_configuration_is_independent_copy", []
        {
            Endpoints endpoints;
            Web::OptionsHandlerConfig config;
            config.insertAllowedMethod("POST");
            endpoints.setEndpointOptions(config);
            config.setAllowedMethods({"GET"});
            endpoints.setEndpointOptions("special", config);
            config.setAllowedMethods({"DELETE"});
            require(endpoints.isOptionsEnabled(), "OPTIONS not enabled");
            require(endpoints.getGlobalOptionsConfig()->getMethodsHeader() == "POST", "Global config aliased source");
            require(endpoints.getOptionsConfigOnEndpoint("special")->getMethodsHeader() == "GET", "Endpoint config aliased source");
            require(endpoints.getOptionsConfigOnEndpoint("missing") == nullptr, "Missing endpoint has override");
            endpoints.setEndpointOptions("special", config);
            require(endpoints.getOptionsConfigOnEndpoint("special")->getMethodsHeader() == "DELETE", "Override not replaced");
        }});
    }
    else if (suite == "filters")
    {
        cases.push_back({"embedded_api_logs_source_file", []
        {
            FilterFile file("<!--<%jfunc/a:GET/v9/missing({})%>-->\n<!--<%jfunc/b:missing({})%>-->");
            Engine engine;
            engine.config.rpcLog = std::make_shared<Mantids30::Runtime::Logs::RPCLog>();
            engine.config.rpcLog->enableColorLogging = false;
            auto socket = std::make_shared<Mantids30::Network::Sockets::Socket_Stream_Dummy>();
            LoggingHandler handler(&engine, socket);
            handler.config = &engine.config;
            const auto output = captureWarnings([&]
            {
                require(Web::HTMLIEngine::processResourceFile(&handler, file.path.string()) == Code::S_200_OK, "Template processing failed");
                handler.handleAPIRequest("/", 1, "POST", "plain", {});
            });
            const std::string marker = "[source=" + file.path.string() + "]";
            const auto first = output.find(marker);
            require(first != std::string::npos && output.find(marker, first + marker.size()) != std::string::npos,
                    "Explicit/default JFUNC calls lost their source file");
            const auto message = output.find("endpoint=plain");
            const auto plain = output.rfind('\n', message);
            require(message != std::string::npos && plain != std::string::npos && output.substr(plain).find("[source=") == std::string::npos,
                    "Template source leaked into a normal API call");
        }});
        cases.push_back({"embedded_api_log_context_restored_after_exception", []
        {
            Engine engine;
            engine.config.rpcLog = std::make_shared<Mantids30::Runtime::Logs::RPCLog>();
            engine.config.rpcLog->enableColorLogging = false;
            auto socket = std::make_shared<Mantids30::Network::Sockets::Socket_Stream_Dummy>();
            LoggingHandler handler(&engine, socket);
            handler.config = &engine.config;
            const auto output = captureWarnings([&]
            {
                handler.handleAPIRequestFromFile("/outer%s.html", "/", 1, "POST", "nested", {});
                handler.log(Mantids30::Runtime::Logs::LogLevel::WARNING, "test", 2048, "plain %s", "message");
            });
            require(output.find("[source=/inner.html] inner failure") != std::string::npos, "Nested source missing");
            require(output.find("[source=/outer%s.html] outer restored") != std::string::npos, "Outer source not restored or percent interpreted");
            const auto message = output.find("plain message");
            const auto plain = output.rfind('\n', message);
            require(message != std::string::npos && plain != std::string::npos && output.substr(plain).find("[source=") == std::string::npos,
                    "Source context survived the embedded API call");
        }});
        cases.push_back({"default_accept_and_clear", []
        {
            Filter filter;
            const auto result = filter.evaluateURI("/anything", {}, {}, false, false);
            require(!result.matched && result.actions.size() == 1 && result.actions[0].type == ActionType::ACCEPT
                        && result.actions[0].statusCode == 200, "Default policy is not accept");
            filter.addFilter(denyFilter());
            require(filter.evaluateURI("/private/file", {}, {}, false, false).matched, "Registered filter did not match");
            filter.clearFilters();
            require(!filter.evaluateURI("/private/file", {}, {}, false, false).matched, "Cleared filter still matches");
        }});
        for (const std::string uri : {"/private/a", "/settings", "/private/", "/private", "/settings/child", "/SETTINGS", "/public"})
        {
            cases.push_back({"regex_full_match." + uri, [uri]
            {
                Filter filter;
                filter.addFilter(denyFilter());
                const bool expected = uri == "/settings" || uri.rfind("/private/", 0) == 0;
                require(filter.evaluateURI(uri, {}, {}, false, false).matched == expected, "Wrong regex URI matching");
            }});
        }
        for (unsigned flags = 0; flags < 64; ++flags)
        {
            cases.push_back({"requirements_matrix." + std::to_string(flags), [flags]
            {
                Filter filter;
                auto rule = denyFilter();
                rule.requiredScopes = {"READ", "WRITE"};
                rule.requiredRoles = {"EDITOR"};
                rule.rejectedScopes = {"BLOCKED"};
                rule.rejectedRoles = {"BANNED"};
                rule.requireSession = true;
                filter.addFilter(rule);
                std::set<std::string> scopes, roles;
                if (flags & 1U)
                {
                    scopes = {"READ", "WRITE"};
                }
                if (flags & 2U)
                {
                    roles.insert("EDITOR");
                }
                if (flags & 4U)
                {
                    scopes.insert("BLOCKED");
                }
                if (flags & 8U)
                {
                    roles.insert("BANNED");
                }
                const bool session = flags & 16U;
                const bool admin = flags & 32U;
                const bool expected = ((flags & 1U) || admin) && ((flags & 2U) || admin)
                    && !(flags & 4U) && !(flags & 8U) && session;
                require(filter.evaluateURI("/settings", scopes, roles, session, admin).matched == expected,
                        "Requirements/admin bypass policy incorrect");
            }});
        }
        cases.push_back({"disallow_session", []
        {
            Filter filter;
            auto rule = denyFilter();
            rule.disallowSession = true;
            filter.addFilter(rule);
            require(filter.evaluateURI("/settings", {}, {}, false, false).matched, "Anonymous rule did not match");
            require(!filter.evaluateURI("/settings", {}, {}, true, true).matched, "Admin bypassed disallowSession");
        }});
        for (const auto terminal : {ActionType::ACCEPT, ActionType::DENY, ActionType::REDIRECT})
        {
            cases.push_back({"terminal_action." + std::to_string(static_cast<int>(terminal)), [terminal]
            {
                Filter filter;
                auto rule = denyFilter();
                Action header;
                header.type = ActionType::ADD_HEADERS;
                header.httpHeaders = {{"X-First", "yes"}};
                rule.actions = {header};
                filter.addFilter(rule);
                Action stop;
                stop.type = terminal;
                rule.actions = {stop, header};
                filter.addFilter(rule);
                filter.addFilter(denyFilter());
                const auto result = filter.evaluateURI("/settings", {}, {}, false, false);
                require(result.matched && result.actions.size() == 2 && result.actions[0].type == ActionType::ADD_HEADERS
                            && result.actions[1].type == terminal, "Terminal action did not stop evaluation in order");
                require(result.toJSON().at("actions").as_array()[0].at("httpHeaders").as_array()[0].at("name") == "X-First",
                        "Action headers missing from JSON diagnostics");
            }});
        }
        cases.push_back({"empty_actions_are_safe", []
        {
            Filter filter;
            auto rule = denyFilter();
            rule.actions.clear();
            filter.addFilter(rule);
            filter.addFilter(denyFilter());
            const auto result = filter.evaluateURI("/settings", {}, {}, false, false);
            require(result.matched && result.actions.size() == 1 && result.actions[0].type == ActionType::DENY,
                    "Empty action list prevented the following filter");
        }});
        cases.push_back({"load_actions_aliases_and_defaults", []
        {
            FilterFile file(R"([{"uriRegexs":["/settings"],"actions":[
                {"type":" add-headers ","headers":{"X-A":"one"}},
                {"type":"replace headers","httpExtraHeaders":{"X-B":"two"}},
                {"type":"process","mode":"htmli"},
                {"type":"process_as_mantids_lang"},
                {"type":"process_as_raw"},
                {"action":"redirect","redirectLocation":"/login","code":307}]}])");
            Filter filter;
            require(filter.loadFiltersFromFile(file.path.string()), "Filter file not loaded");
            const auto result = filter.evaluateURI("/settings", {}, {}, false, false);
            require(result.actions.size() == 6, "Actions missing from file");
            require(result.actions[0].httpHeaders[0] == std::make_pair(std::string("X-A"), std::string("one")), "Header parsing failed");
            require(result.actions[1].type == ActionType::REPLACE_HEADERS && result.actions[1].httpHeaders[0].first == "X-B", "Header alias failed");
            require(result.actions[2].processingMode == Filter::ProcessingMode::HTMLIENGINE
                        && result.actions[3].processingMode == Filter::ProcessingMode::MANTIDSLANG
                        && result.actions[4].processingMode == Filter::ProcessingMode::RAW, "Processing aliases failed");
            require(result.actions[5].redirectLocation == "/login" && result.actions[5].statusCode == 307, "Redirect aliases failed");
        }});
        for (const std::string text : {"{", R"([{"uriRegexs":["["],"actions":["deny"]}])",
                 R"([{"actions":[{"type":"unknown"}]}])", R"([{"actions":[{"type":"deny","statusCode":65536}]}])"})
        {
            cases.push_back({"invalid_file_is_atomic." + text, [text]
            {
                Filter filter;
                FilterFile initial(R"([{"uriRegexs":["/public"],"actions":["deny"]}])");
                FilterFile invalid("[{\"uriRegexs\":[\"/settings\"],\"actions\":[\"deny\"]},"
                                   + text.substr(text.front() == '[' ? 1 : 0));
                require(filter.loadFiltersFromFile(initial.path.string()), "Initial filter load failed");
                bool threw = false;
                try
                {
                    filter.loadFiltersFromFile(invalid.path.string());
                }
                catch (const std::exception &)
                {
                    threw = true;
                }
                require(threw, "Invalid filter configuration accepted");
                require(filter.evaluateURI("/public", {}, {}, false, false).matched
                            && !filter.evaluateURI("/settings", {}, {}, false, false).matched, "Failed load partially changed filters");
            }});
        }
        cases.push_back({"document_root_autoload", []
        {
            FilterFile file(R"([{"uriRegexs":["/private/.*"],"actions":["deny"]}])");
            Web::APIServerConfig config;
            require(config.setDocumentRootPath(file.directory.string()), "Document root rejected");
            require(config.resourceFilter && config.resourceFilter->evaluateURI("/private/file", {}, {}, false, false).matched,
                    "resources.conf was not autoloaded");
            require(!config.setDocumentRootPath((file.directory / "missing").string()), "Nonexistent root accepted");
            require(config.getDocumentRootPath() == file.directory.string(), "Invalid path replaced document root");
        }});
    }
    return cases;
}

} // namespace WebRESTFulTests
