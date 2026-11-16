#include "test_support.h"

#include <atomic>
#include <ctime>
#include <thread>

namespace WebRESTFulTests {
namespace {

struct WebSocketProbe
{
    static inline size_t calls = 0;
    static inline boost::json::value json;
    static inline std::string raw;
    static inline Web::Sessions::ClientDetails details;

    static void invoke(void *, const std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> &content,
                       const boost::json::value &value, const Web::WebSocket::WebSocketParameters &,
                       Web::Sessions::ClientDetails &client)
    {
        ++calls;
        json = value;
        raw = content->toString().value();
        details = client;
    }
};

} // namespace

std::vector<Case> stateCases(const std::string &suite)
{
    std::vector<Case> cases;
    if (suite == "sessions")
    {
        cases.push_back({"identity_and_token_snapshot", []
        {
            JWT::Token token;
            token.setSubject("account-123");
            token.setDomain("example.test");
            token.setClaim("impersonator", "admin-456");
            token.setJwtId("session-789");
            token.addScope("READ");
            const auto before = std::time(nullptr);
            Web::Sessions::Session session(token);
            const auto after = std::time(nullptr);
            token.setSubject("changed");
            require(session.getUser() == "account-123" && session.getDomain() == "example.test"
                        && session.getImpersonator() == "admin-456", "Session identity was not snapshotted");
            require(session.getFirstActivity() >= before && session.getFirstActivity() <= after, "Wrong creation time");
            auto copy = session.getJWTAuthenticatedInfo();
            require(copy.getJwtId() == "session-789" && copy.hasScope("READ"), "Session lost JWT claims");
            copy.setSubject("changed-again");
            require(session.getJWTAuthenticatedInfo().getSubject() == "account-123", "Returned token aliases session state");
        }});
        cases.push_back({"activity_expiration_without_sleep", []
        {
            Web::Sessions::Session session(JWT::Token{});
            const auto first = session.getFirstActivity();
            const auto now = std::time(nullptr);
            session.setLastActivity(now - 1000);
            require(session.getLastActivity() == now - 1000 && session.isLastActivityExpired(100), "Old activity did not expire");
            require(!session.isLastActivityExpired(2000), "Recent activity expired");
            session.setLastActivity(now + 1000);
            require(session.isLastActivityExpired(2000), "Future clock anomaly not detected");
            const auto before = std::time(nullptr);
            session.updateLastActivity();
            require(session.getLastActivity() >= before && session.getLastActivity() <= std::time(nullptr), "Activity not updated");
            require(session.getFirstActivity() == first, "Update changed creation timestamp");
        }});
        cases.push_back({"variables_crud_null_and_value_copy", []
        {
            Web::Sessions::Session_Vars vars;
            require(!vars.doesSessionVariableExist("missing") && vars.getSessionVariableValue("missing").is_null(), "Missing variable is not null");
            require(!vars.doesSessionVariableExist("missing"), "Reading inserted missing variable");
            boost::json::value value = boost::json::object{{"items", boost::json::array{1, "two", nullptr}}};
            vars.setSessionVariable("data", value);
            value.at("items").as_array().clear();
            auto copy = vars.getSessionVariableValue("data");
            require(copy.at("items").as_array().size() == 3, "Setting variable did not copy input");
            copy.at("items").as_array().clear();
            require(vars.getSessionVariableValue("data").at("items").as_array().size() == 3, "Returned value aliases storage");
            vars.setSessionVariable("data", false);
            require(vars.getSessionVariableValue("data") == false, "Variable overwrite failed");
            vars.setSessionVariable("null", nullptr);
            require(vars.doesSessionVariableExist("null") && vars.getSessionVariableValue("null").is_null(), "Stored null treated as missing");
            vars.clearSessionVariable("data");
            vars.clearSessionVariable("missing");
            require(!vars.doesSessionVariableExist("data") && vars.doesSessionVariableExist("null"), "Clear removed wrong variable");
        }});
        cases.push_back({"concurrent_variables_and_activity", []
        {
            Web::Sessions::Session session(JWT::Token{});
            std::atomic<bool> good{true};
            std::vector<std::thread> threads;
            for (unsigned worker = 0; worker < 8; ++worker)
            {
                threads.emplace_back([&, worker]
                {
                    const auto key = "worker-" + std::to_string(worker);
                    for (unsigned iteration = 0; iteration < 250; ++iteration)
                    {
                        session.setSessionVariable(key, iteration);
                        if (!session.doesSessionVariableExist(key) || session.getSessionVariableValue(key) != iteration)
                        {
                            good = false;
                        }
                        session.updateLastActivity();
                        (void)session.getSessionVariableValue("shared");
                        session.setSessionVariable("shared", worker);
                        session.clearSessionVariable("shared");
                    }
                });
            }
            for (auto &thread : threads)
            {
                thread.join();
            }
            require(good, "Concurrent variable update lost thread-local state");
            for (unsigned worker = 0; worker < 8; ++worker)
            {
                require(session.getSessionVariableValue("worker-" + std::to_string(worker)) == 249, "Final concurrent value missing");
            }
        }});
        cases.push_back({"client_details_json", []
        {
            Web::Sessions::ClientDetails details;
            details.ipAddress = "2001:db8::1";
            details.userAgent = "curl/8.0";
            details.tlsCommonName = "client.test";
            details.extraData = "private";
            require(details.toJSON() == boost::json::object{{"ip", "2001:db8::1"}, {"userAgent", "curl/8.0"},
                        {"tlsCommonName", "client.test"}}, "Client details serialization changed");
        }});
    }
    else if (suite == "websocket")
    {
        for (unsigned received = 0; received < 8; ++received)
        {
            cases.push_back({"default_security." + std::to_string(received), [received]
            {
                Web::WebSocket::Endpoints endpoints;
                require(endpoints.addEndpoint("/events", Web::WebSocket::Endpoint{}), "WebSocket registration failed");
                Web::WebSocket::Endpoints::SecurityParameters auth{bool(received & 1U), bool(received & 2U), bool(received & 4U)};
                using WSResult = Web::WebSocket::Endpoints::HandleResult;
                const auto expected = received == 7 ? WSResult::SUCCESS : WSResult::AUTHENTICATION_REQUIRED;
                require(endpoints.checkEndpoint("/events", {}, false, auth) == expected, "WebSocket security check incorrect");
                require(endpoints.checkEndpoint("/events", {}, true, auth) == expected, "Admin bypassed WebSocket authentication");
                require(endpoints.checkEndpoint("/missing", {}, true, auth) == WSResult::ENDPOINT_NOT_FOUND, "Missing endpoint accepted");
            }});
        }
        using Event = HTTP::WebSocket::EventType;
        for (const auto event : {Event::SESSION_START, Event::RECEIVED_MESSAGE_TEXT, Event::RECEIVED_MESSAGE_BINARY, Event::SESSION_END})
        {
            cases.push_back({"event_dispatch." + std::to_string(static_cast<int>(event)), [event]
            {
                Engine engine;
                auto endpoints = std::make_shared<Web::WebSocket::Endpoints>();
                engine.setWebsocketEndpoints(endpoints);
                Web::WebSocket::Endpoint endpoint;
                endpoint.sessionStartHandler = WebSocketProbe::invoke;
                endpoint.textMessageReceivedHandler = WebSocketProbe::invoke;
                endpoint.binaryMessageReceivedHandler = WebSocketProbe::invoke;
                endpoint.sessionEndHandler = WebSocketProbe::invoke;
                endpoints->addEndpoint("/events", endpoint);
                using WSResult = Web::WebSocket::Endpoints::HandleResult;
                require(endpoints->checkEndpoint("/events", {}, false, {true, true, true}) == WSResult::SUCCESS, "Endpoint check failed");
                Fixture f;
                Web::WebSocket::WebSocketParameters parameters;
                parameters.clientRequest = &f.request;
                auto content = std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>();
                const std::string raw = R"({"message":"hello"})";
                require(bool(content->append(raw.data(), raw.size())), "Cannot prepare WebSocket payload");
                WebSocketProbe::calls = 0;
                require(endpoints->handleEvent(event, content, parameters) == WSResult::SUCCESS && WebSocketProbe::calls == 1,
                        "Wrong WebSocket event callback");
                require(WebSocketProbe::raw == raw && WebSocketProbe::details.ipAddress == "192.0.2.4", "WebSocket data/metadata lost");
                const bool translates = event == Event::RECEIVED_MESSAGE_TEXT || event == Event::SESSION_END;
                require(WebSocketProbe::json == (translates ? boost::json::parse(raw) : boost::json::value{}), "Wrong event JSON translation");
                require(endpoints->handleEvent(static_cast<Event>(255), content, parameters) == WSResult::INVALID_EVENT_TYPE
                            && WebSocketProbe::calls == 1, "Invalid event invoked callback");
                engine.config.webSockets.translateWebSocketTextMessagesToJSON = false;
                require(endpoints->handleEvent(Event::RECEIVED_MESSAGE_TEXT, content, parameters) == WSResult::SUCCESS
                            && WebSocketProbe::json.is_null(), "JSON translation toggle ignored");
                require(endpoints->checkEndpoint("/missing", {}, false, {}) == WSResult::ENDPOINT_NOT_FOUND, "Missing URI accepted");
                require(endpoints->handleEvent(event, content, parameters) == WSResult::ENDPOINT_NOT_FOUND, "Missing URI retained previous callback");
            }});
        }
        cases.push_back({"registry_lookup_and_empty_connections", []
        {
            Web::WebSocket::Endpoints endpoints;
            endpoints.addEndpoint("/events", Web::WebSocket::Endpoint{});
            const auto endpoint = endpoints.getWebSocketEndpointByURI("/events");
            require(endpoint && !endpoints.getWebSocketEndpointByURI("/events/"), "Endpoint lookup not exact");
            require(endpoint->getActiveUserConnectionsCount("nobody") == 0, "Empty endpoint has user connections");
            require(!endpoint->sendJSONToConnectionID("missing", nullptr), "Sent to nonexistent connection");
            require(endpoint->sendJSONToSubscriptionTopic("missing", nullptr) == 0, "Sent to nonexistent subscription");
        }});
    }
    return cases;
}

} // namespace WebRESTFulTests
