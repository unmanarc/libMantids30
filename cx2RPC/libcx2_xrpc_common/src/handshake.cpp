#include "handshake.h"

using namespace CX2::RPC::XRPC;
using namespace CX2;

Handshake::Handshake()
{
    clear();
    setParseMode(Memory::Streams::Parsing::PARSE_MODE_DELIMITER);
    setParseDelimiter("\n");
    setParseDataTargetSize(256*KB_MULT); // Max message size per line: 256K.
}

bool Handshake::stream(Memory::Streams::Status &wrStat)
{
    if (!upStream->writeString(protocolVersion,wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!upStream->writeString(serverInfo,wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!upStream->writeString(serverVersionMajor,wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!upStream->writeString(serverVersionMinor,wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!upStream->writeString(productInfo,wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!upStream->writeString(productVersion,wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    return true;
}

void Handshake::clear()
{
    protocolVersion = "JSON_ASYNC_RPC_1_0";
    serverInfo = "JSON Async RPC Server";
    serverVersionMajor = "1";
    serverVersionMinor = "0";

    productInfo = "Unknown Product";
    productVersion = "UNK_1_0";
    curHandShakeVal = E_HANDSHAKE_PROTO_VERSION;
}

std::string Handshake::getProductInfo() const
{
    return productInfo;
}

void Handshake::setProductInfo(const std::string &value)
{
    productInfo = value;
}

std::string Handshake::getProductVersion() const
{
    return productVersion;
}

void Handshake::setProductVersion(const std::string &value)
{
    productVersion = value;
}

std::string Handshake::getServerInfo() const
{
    return serverInfo;
}

void Handshake::setServerInfo(const std::string &value)
{
    serverInfo = value;
}

std::string Handshake::getServerVersionMajor() const
{
    return serverVersionMajor;
}

void Handshake::setServerVersionMajor(const std::string &value)
{
    serverVersionMajor = value;
}

std::string Handshake::getServerVersionMinor() const
{
    return serverVersionMinor;
}

void Handshake::setServerVersionMinor(const std::string &value)
{
    serverVersionMinor = value;
}

std::string Handshake::getProtocolVersion() const
{
    return protocolVersion;
}

void Handshake::setProtocolVersion(const std::string &value)
{
    protocolVersion = value;
}

Memory::Streams::Parsing::ParseStatus Handshake::parse()
{
    //printf("parsing data %d with string: %s\n",curHandShakeVal, getParsedData()->toString().c_str() ); fflush(stdout);
    switch(curHandShakeVal)
    {
    case E_HANDSHAKE_PROTO_VERSION:
    {
        protocolVersion = getParsedData()->toString();
        curHandShakeVal=E_HANDSHAKE_SERVER_INFO;
    } break;
    case E_HANDSHAKE_SERVER_INFO:
    {
        serverInfo = getParsedData()->toString();
        curHandShakeVal=E_HANDSHAKE_SERVER_VER_MAJ;
    } break;
    case E_HANDSHAKE_SERVER_VER_MAJ:
    {
        serverVersionMajor = getParsedData()->toString();
        curHandShakeVal=E_HANDSHAKE_SERVER_VER_MIN;
    } break;
    case E_HANDSHAKE_SERVER_VER_MIN:
    {
        serverVersionMinor = getParsedData()->toString();
        curHandShakeVal=E_HANDSHAKE_PROD_INFO;
    } break;
    case E_HANDSHAKE_PROD_INFO:
    {
        productInfo = getParsedData()->toString();
        curHandShakeVal=E_HANDSHAKE_PROD_VERSION;
    } break;
    case E_HANDSHAKE_PROD_VERSION:
    {
        productVersion = getParsedData()->toString();
        curHandShakeVal=E_HANDSHAKE_PROTO_VERSION;
        return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
    }
    }

    return Memory::Streams::Parsing::PARSE_STAT_GET_MORE_DATA;
}
