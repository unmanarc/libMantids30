#ifndef HTTP1BASE_H
#define HTTP1BASE_H

#include <cx2_mem_vars/streamparser.h>
#include <cx2_mem_vars/vars.h>

#include "req_requestline.h"
#include "common_content.h"
#include "rsp_status.h"
#include <cx2_netp_mime/mime_sub_header.h>

//#include <netinet/in.h>

#define VNET_PROD_NAME "vProtonHTTP"

#define VNET_HTTP_VERSION_MAJOR 0
#define VNET_HTTP_VERSION_MINOR 1

namespace CX2 { namespace Network { namespace HTTP {

namespace Request {
struct RequestDeliveryObjects
{
    RequestLine * request;
    Common::Content * content;
    MIME::MIME_Sub_Header * headers;
};
}

namespace Response {
struct ResponseDeliveryObjects
{
    Response::Status * status;
    Common::Content * content;
    MIME::MIME_Sub_Header * headers;
};
}

class HTTPv1_Base : public Memory::Streams::Parsing::Parser
{
public:
    HTTPv1_Base(bool clientMode, Memory::Streams::Streamable *sobject);
    virtual ~HTTPv1_Base()  override {}

    // Parameters:
    Response::ResponseDeliveryObjects response();
    Request::RequestDeliveryObjects request();

protected:
    virtual bool initProtocol() override;
    virtual void endProtocol() override;

    virtual void * getThis()=0;
    virtual bool changeToNextParser()  override = 0;
    /**
     * @brief code Response - Server code response. (HTTP Version, Response code, message)
     */
    Response::Status _serverCodeResponse;
    /**
     * @brief headers - Options Values.
     */
    MIME::MIME_Sub_Header _clientHeaders;
    /**
     * @brief content - Content Data.
     */
    Common::Content _clientContentData;
    /**
     * @brief clientRequest - URL Request (Request type, URL, GET Vars, and HTTP version)
     */
    Request::RequestLine _clientRequestLine;
    /**
     * @brief headers - Options Values.
     */
    MIME::MIME_Sub_Header _serverHeaders;
    /**
     * @brief content - Content Data.
     */
    Common::Content _serverContentData;

private:
    void setInternalProductVersion(const std::string & prodName, const std::string & extraInfo, const uint32_t &versionMajor = VNET_HTTP_VERSION_MAJOR, const uint32_t &versionMinor = VNET_HTTP_VERSION_MINOR);

};

}}}

#endif // HTTP1BASE_H
