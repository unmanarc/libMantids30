#include "httpv1_base.h"

using namespace Mantids::Network::HTTP;
using namespace Mantids;

HTTPv1_Base::HTTPv1_Base(bool clientMode, Memory::Streams::Streamable *sobject) : Memory::Streams::Parsing::Parser(sobject,clientMode)
{
    initSubParser(&_clientRequestLine);
    initSubParser(&_clientHeaders);
    initSubParser(&_clientContentData);

    initSubParser(&_serverCodeResponse);
    initSubParser(&_serverHeaders);
    initSubParser(&_serverContentData);
    setInternalProductVersion("Mantids::HTTP","(+https://github.com/unmanarc/cxFramework2)");
}

Response::ResponseDeliveryObjects HTTPv1_Base::response()
{
    Response::ResponseDeliveryObjects ret;
    ret.status = &_serverCodeResponse;
    ret.content = &_serverContentData;
    ret.headers = &_serverHeaders;
    return ret;
}

Request::RequestDeliveryObjects HTTPv1_Base::request()
{
    Request::RequestDeliveryObjects ret;
    ret.content = &_clientContentData;
    ret.headers = &_clientHeaders;
    ret.request = &_clientRequestLine;
    return ret;
}

bool HTTPv1_Base::initProtocol()
{
    return true;
}

void HTTPv1_Base::endProtocol()
{

}

void HTTPv1_Base::setInternalProductVersion(const std::string &prodName, const std::string &extraInfo, const uint32_t &versionMajor, const uint32_t &versionMinor)
{
    _serverHeaders .replace("Server",
                prodName + "/" + std::to_string(versionMajor) + "." + std::to_string(versionMinor) +
                (!extraInfo.empty()? (" " + extraInfo) :"") );
}
