#include "httpv1_base.h"

using namespace Mantids30::Protocols;
using namespace Mantids30;

Mantids30::Protocols::HTTP::HTTPv1_Base::HTTPv1_Base(bool clientMode, const std::shared_ptr<StreamableObject> &sobject)
    : Mantids30::Core::Memory::Streams::Parser(sobject, clientMode)
{
    initSubParser(&clientRequest.requestLine);
    initSubParser(&clientRequest.headers);
    initSubParser(&clientRequest.content);

    initSubParser(&serverResponse.status);
    initSubParser(&serverResponse.headers);
    initSubParser(&serverResponse.content);

    if (!clientMode)
    {
        setInternalProductVersion("Mantids30::HTTP", "(+https://github.com/unmanarc/libMantids30)");
    }
}

bool Mantids30::Protocols::HTTP::HTTPv1_Base::initProtocol()
{
    return true;
}

void Mantids30::Protocols::HTTP::HTTPv1_Base::endProtocol() {}

void Mantids30::Protocols::HTTP::HTTPv1_Base::setInternalProductVersion(std::string_view prodName, std::string_view extraInfo, const uint32_t &versionMajor, const uint32_t &versionMinor)
{
    serverResponse.headers.replace("Server", std::string(prodName) + "/" + std::to_string(versionMajor) + "." + std::to_string(versionMinor) + (!extraInfo.empty() ? (" " + std::string(extraInfo)) : ""));
}
