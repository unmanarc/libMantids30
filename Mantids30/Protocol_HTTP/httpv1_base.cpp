#include "httpv1_base.h"

using namespace Mantids30::Network::Protocols;
using namespace Mantids30;

HTTP::HTTPv1_Base::HTTPv1_Base(bool clientMode, std::shared_ptr<StreamableObject> sobject) : Memory::Streams::Parser(sobject,clientMode)
{
    initSubParser(&clientRequest.requestLine);
    initSubParser(&clientRequest.headers);
    initSubParser(&clientRequest.content);

    initSubParser(&serverResponse.status);
    initSubParser(&serverResponse.headers);
    initSubParser(&serverResponse.content);

    if ( !clientMode )
        setInternalProductVersion("Mantids30::HTTP","(+https://github.com/unmanarc/libMantids30)");
}

bool HTTP::HTTPv1_Base::initProtocol()
{
    return true;
}

void HTTP::HTTPv1_Base::endProtocol()
{
}

void HTTP::HTTPv1_Base::setInternalProductVersion(const std::string &prodName, const std::string &extraInfo, const uint32_t &versionMajor, const uint32_t &versionMinor)
{
    serverResponse.headers.replace("Server",
                prodName + "/" + std::to_string(versionMajor) + "." + std::to_string(versionMinor) +
                (!extraInfo.empty()? (" " + extraInfo) :"") );
}
