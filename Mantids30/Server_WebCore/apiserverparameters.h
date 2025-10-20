#ifndef APISERVERPARAMETERS_H
#define APISERVERPARAMETERS_H

#include <Mantids30/API_RESTful/endpointshandler.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Program_Logs/applog.h>
#include <Mantids30/Program_Logs/rpclog.h>
#include <Mantids30/Program_Logs/weblog.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Sessions/session.h>

#include "resourcesfilter.h"
#include <memory>
#include <set>
#include <string>

namespace Mantids30 {
namespace Network {
namespace Servers {
namespace Web {

class APIServerParameters
{
public:
    using ClientDetails = Mantids30::Sessions::ClientDetails;
    using RequestParameters = Mantids30::API::RESTful::RequestParameters;
    using APIReturn = Mantids30::API::APIReturn;

    APIServerParameters() = default;
    ~APIServerParameters();

    /**
     * @brief Callback function type for dynamic validation of access tokens.
     * This function is used when the application needs to validate an access token
     * based on the provided API key. It allows for different validation logic for
     * different applications or scenarios.
     *
     * @param token The access token to validate.
     * @param apikey The API key associated with the request.
     * @param accessToken A reference to the parsed JWT token, if valid.
     * @return true if the token is valid for the given API key, false otherwise.
     */
    typedef bool (*DynamicTokenValidatorFunction)(const std::string &token, const std::string &apikey, Mantids30::DataFormat::JWT::Token *accessToken);

    /**
     * @brief The dynamic validator callback function.
     * Set this to a custom validation function to handle dynamic token validation.
     */
    DynamicTokenValidatorFunction dynamicTokenValidator = nullptr;

    /**
     * @brief Callback function type for dynamic validation of the origin header.
     * This function allows applications to validate the origin header of incoming requests
     * based on the provided API key. It is useful for ensuring requests that come from different trusted
     * sources and different applications.
     *
     * @param origin The origin header value from the request.
     * @param apikey The API key associated with the request.
     * @return true if the origin is valid for the given API key, false otherwise.
     */
    typedef bool (*DynamicOriginValidatorFunction)(const std::string &origin, const std::string &apikey);

    /**
     * @brief The dynamic origin validator callback function.
     * Set this to a custom validation function to handle dynamic origin validation.
     */
    DynamicOriginValidatorFunction dynamicOriginValidator = nullptr;

    // JWT Validator and signer...
    std::shared_ptr<DataFormat::JWT> jwtValidator;
    std::shared_ptr<DataFormat::JWT> jwtSigner;

    std::shared_ptr<Program::Logs::AppLog> appLog = nullptr;
    std::shared_ptr<Program::Logs::RPCLog> rpcLog = nullptr;
    std::shared_ptr<Program::Logs::WebLog> webLog = nullptr;

    /**
     * @brief redirections HTTP 307 Redirections (Temporary Redirect), by example, you can use some path for URL shortening here.
     */
    std::map<std::string, std::string> redirections;

    /**
     * @brief Sets the shared pointer resource filter for the web session.
     *
     * Configures a filter for regular files that can be matched with the current
     * session and additional conditions. This allows for more fine-grained control
     * over which resources can be accessed during a session.
     *
     */
    std::shared_ptr<API::Web::ResourcesFilter> resourceFilter;

    /**
     * @brief List of allowed origins for login requests.
     *
     * This set contains the origins that are permitted to initiate login requests. It is used to
     * ensure that only requests coming from specified trusted origins are processed for login,
     * enhancing security by preventing unauthorized cross-origin requests.
     */
    std::set<std::string> permittedLoginOrigins;

    /**
     * @brief callbackAPIEndpointName The method name used for JWT Token absorption from the IAM.
     */
    std::string callbackAPIEndpointName = "callback";

    /**
     * @brief useJSTokenCookie for RESTful server, JS Token cookie means that the JS will receive the JWT token that can be used for Header authentication via Cookie
     */
    bool useJSTokenCookie = false;

    /**
     * @brief allowFloatingClients Allow clients to change their IP Address...
     */
    bool allowFloatingClients = false;

    /**
     * @brief allowImpersonation if true, allow impersonations from admin's
     */
    bool allowImpersonation = false;

    /**
     * @brief allowImpersonationBetweenDifferentDomains
     */
    bool allowImpersonationBetweenDifferentDomains = false;

    /**
     * @brief autoloadResourcesFilter The resource filter will be overwritten by the resources.conf if present if this variable is on.
     */
    bool autoloadResourcesFilter = true;

    /**
     * @brief Sets the flag for formatted JSON output.
     *
     * Enables or disables formatted (pretty-printed) JSON output based on the provided value.
     */
    bool useFormattedJSONOutput = true;

    /**
     * @brief Enables or disables the HTMLI pre-processing engine.
     *
     * Activates the internal HTMLI engine, which preprocesses HTML content to handle includes, variables, and other custom directives.
     */
    bool useHTMLIEngine = true;

    /**
     * @brief APIURLs path's where the API will be available
     */
    std::set<std::string> APIURLs = {"/api"};

    /**
     * @brief appName The application name (eg. IAM) identificator.
     */
    std::string appName;

    /**
     * @brief softwareName Software name for our API server
     */
    std::string softwareName;

    /**
     * @brief Sets the redirect path for 404 (Not Found) errors.
     *
     * Specifies the URL path to which users will be redirected when a 404 error occurs.
     */
    std::string redirectPathOn404 = "/";

    /**
     * @brief defaultLoginRedirect URL for login redirection called by /auth/login_redirect
     */
    std::string defaultLoginRedirect;

    /**
     * @brief List of allowed origins for API requests.
     *
     * This set contains the origins that are permitted to initiate API requests. It is used to
     * ensure that only requests coming from specified trusted origins are processed for API requests,
     * enhancing security by preventing unauthorized cross-origin requests.
     */
    std::set<std::string> permittedAPIOrigins;

    /**
     * @brief Sets the redirect path for failed login attempts.
     *
     * Specifies the URL path to which users will be redirected when a login attempt fails.
     *
     */
    std::string redirectLocationOnLoginFailed = "/";

    /**
     * @brief Sets the web server name for the HTTP header.
     *
     * Updates the server name that will be included in the HTTP response headers.
     */
    std::string webServerName;

    /**
     * @brief softwareVersion Set Software Version (to display in the API)
     */
    std::string softwareVersion;

    /**
     * @brief Sets the version number of the web server software. (to display in the API)
     *
     * This function sets the version number of the web server software. The version number is represented by three
     * integers: the major version, the minor version, and the subminor version. The subText parameter allows for a string
     * to be appended to the end of the version number, which can be used to provide additional information about the
     * version, such as a release code or build number.
     *
     * @param major The major version number.
     * @param minor The minor version number.
     * @param subminor The subminor version number.
     * @param subText A string to append to the end of the version number.
     *
     * @return void.
     */
    void setSoftwareVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string &subText);

    /**
     * @brief softwareDescription Set the software description (to be sent in the API)
     */
    std::string softwareDescription;

    /**
     * @brief Typedef for mapping paths to dynamic request handlers.
     *
     * Defines a type alias `DynamicRequestHandler` for the function pointer used to handle
     * dynamic HTTP requests. This alias can be used within a `std::map` to map
     * `std::string` keys (representing paths) to the corresponding handler functions.
     *
     * Each handler function takes the following parameters:
     * - `internalPath` (const std::string&): The internal path that matches the dynamic content path.
     * - `request` (HTTPv1_Base::Request*): Pointer to the HTTP request object.
     * - `response` (HTTPv1_Base::Response*): Pointer to the HTTP response object.
     *
     * The handler returns a value of type `Protocols::HTTP::Status::Codes` representing the
     * response status code.
     */
    typedef Protocols::HTTP::Status::Codes (*DynamicRequestHandler)(const std::string &internalPath,
                                                                    Mantids30::Network::Protocols::HTTP::HTTPv1_Base::Request *request,
                                                                    Mantids30::Network::Protocols::HTTP::HTTPv1_Base::Response *response,
                                                                    std::shared_ptr<void> obj);

    struct DynamicRequestHandlerDef
    {
        DynamicRequestHandler handler;
        std::shared_ptr<void> obj = nullptr;
    };

    /**
     * @brief Map for associating URL paths with dynamic request handler functions.
     *
     * The `m_dynamicRequestHandlers` is a `std::map` that associates a string representing
     * a URL path with a `DynamicRequestHandler` function pointer. This allows dynamic HTTP
     * requests to be routed and handled based on the specific URL path requested by the client.
     *
     * - **Key (`std::string`)**: Represents the URL path segment. When an HTTP request's path
     *   matches one of the keys in the map, the corresponding handler function is invoked.
     *
     * - **Value (`DynamicRequestHandler`)**: A function pointer that points to the handler
     *   function responsible for processing the request and writing the response. The function
     *   must match the `DynamicRequestHandler` signature:
     *   - `const std::string& internalPath`: The internal path within the server that matches
     *     the key in the map.
     *   - `HTTPv1_Base::Request* request`: Pointer to the HTTP request object, containing
     *     information about the client request.
     *   - `HTTPv1_Base::Response* response`: Pointer to the HTTP response object, used to
     *     generate the response to the client.
     *   - **Return**: A `Protocols::HTTP::Status::Codes` representing the status of the
     *     response, such as success or error codes.
     *
     * This map enables efficient routing by allowing the server to quickly look up and execute
     * the appropriate handler for any given path. If a request's path matches an entry in the
     * map, the corresponding handler function is called to process the request.
     */
    std::map<std::string, DynamicRequestHandlerDef> dynamicRequestHandlersByRoute;

    /**
     * @brief Sets the document root path for serving web resources.
     *
     * This function sets the path to the document root directory, from which static web
     * resources will be served. It validates the provided path for read access and
     * optionally loads a resources filter configuration if configured to do so.
     *
     * @param value A string representing the path to the document root directory.
     *
     * @return `true` if the path is successfully set and valid, `false` if the path
     * does not have read access.
     */
    bool setDocumentRootPath(const std::string &value);
    /**
     * @brief Adds a static content element to the web server.
     *
     * This function adds a static content element to the web server. The static content element is identified by its path
     * and its content. The path parameter represents the location of the content element, while the content parameter
     * represents the actual content that will be displayed.
     *
     * @param path The path of the static content element to be added.
     * @param content The content of the static content element to be added.
     *
     * @return void.
     */
    void addStaticContentElement(const std::string &path, const std::string &content);

    /**
     * @brief Adds an overlapped directory mapping for internal paths to filesystem paths.
     *
     * This function allows the application to define a mapping between an internal path
     * and a filesystem path. When a request is made to an internal path that matches one
     * of these mappings, the server will serve content from the corresponding filesystem path.
     *
     * @param internalPath The internal path used for routing (e.g., "/static").
     * @param fsPath The filesystem path to serve content from (e.g., "/var/www/static").
     *
     * @return true if the mapping was successfully added, false otherwise.
     */
    bool addOverlappedDirectory(std::string internalPath, std::string fsPath);


    /**
     * @brief Get the list of overlapped directories.
     *
     * Returns a const reference to the list of overlapped directory mappings.
     *
     * @return const std::list<std::pair<std::string, std::string>>& The list of overlapped directories.
     */
    const std::list<std::pair<std::string, std::string>>& getOverlappedDirectories() const;


    /**
     * @brief getStaticContentElements Get static content element...
     * @return
     */
    std::map<std::string, std::shared_ptr<Mantids30::Memory::Containers::B_MEM>> getStaticContentElements();

    std::string getDocumentRootPath() const;

    bool isDocumentRootPathAccesible() const;

private:
    /**
     * @brief Sets the document root path for serving files.
     *
     * Specifies the root directory path from which static files will be served.
     * This path is used as the base directory for handling incoming requests that
     * are mapped to file resources.
     */
    std::string m_documentRootPath;

    /**
     * @brief Map of overlapped directories for internal paths to filesystem paths.
     *
     * This map stores the mapping between internal paths (used for routing within the application)
     * and filesystem paths (where the actual files are stored). It allows serving static content
     * from specific filesystem locations under designated internal paths, enabling flexible content
     * delivery and directory structure management.
     */
    std::list<std::pair<std::string, std::string>> m_overlappedDirectories;

    std::map<std::string, std::shared_ptr<Mantids30::Memory::Containers::B_MEM>> m_staticContentElements;
    std::mutex m_internalContentMutex;
    std::list<char *> m_memToBeFreed;
};

} // namespace Web
} // namespace Servers
} // namespace Network
} // namespace Mantids30

#endif // APISERVERPARAMETERS_H
