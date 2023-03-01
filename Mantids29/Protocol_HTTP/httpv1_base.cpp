#include "httpv1_base.h"

using namespace Mantids29::Protocols::HTTP;
using namespace Mantids29;

HTTPv1_Base::HTTPv1_Base(bool clientMode, Memory::Streams::StreamableObject *sobject) : Memory::Streams::Parser(sobject,clientMode)
{
    initSubParser(&clientRequest.requestLine);
    initSubParser(&clientRequest.headers);
    initSubParser(&clientRequest.content);

    initSubParser(&serverResponse.status);
    initSubParser(&serverResponse.headers);
    initSubParser(&serverResponse.content);
    setInternalProductVersion("Mantids29::HTTP","(+https://github.com/unmanarc/libMantids29)");
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
    serverResponse.headers.replace("Server",
                prodName + "/" + std::to_string(versionMajor) + "." + std::to_string(versionMinor) +
                (!extraInfo.empty()? (" " + extraInfo) :"") );
}
