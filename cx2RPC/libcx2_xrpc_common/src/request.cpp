#include "request.h"
using namespace CX2::RPC::XRPC;
using namespace CX2;

Request::Request()
{
    clear();
    setParseMode(Memory::Streams::Parsing::PARSE_MODE_DELIMITER);
    setParseDelimiter("\n");
    setParseDataTargetSize(256*KB_MULT); // Max message size per line: 256K.
}

Memory::Streams::Parsing::ParseStatus Request::parse()
{
    switch(curProcVal)
    {
    case E_REQ_METHOD_NAME:
    {
        methodName = getParsedData()->toString();
        curProcVal = E_REQ_METHOD_RPCMODE;
    }break;
    case E_REQ_METHOD_RPCMODE:
    {
        rpcMode = getParsedData()->toString();
        curProcVal = E_REQ_METHOD_PAYLOAD;
    }break;
    case E_REQ_METHOD_PAYLOAD:
    {
        Memory::Streams::Status stat;
        payload.clear();
        if (!getParsedData()->streamTo(&payload,stat))
            return Memory::Streams::Parsing::PARSE_STAT_ERROR;
        curProcVal = E_REQ_METHOD_IDS;
    }break;
    case E_REQ_METHOD_IDS:
    {
        Memory::Streams::Status stat;
        ids.clear();

        /*std::cout << "ids parsed:" << getParsedData()->toString() << std::endl << std::flush;
        std::cout << "old ids:" << ids.getString() << std::endl << std::flush;*/
        if (!getParsedData()->streamTo(&ids,stat))
            return Memory::Streams::Parsing::PARSE_STAT_ERROR;
        curProcVal = E_REQ_METHOD_EXTRAINFO;
        //std::cout << "new ids:" << ids.getString() << std::endl << std::flush;

    }break;
    case E_REQ_METHOD_EXTRAINFO:
    {
        Memory::Streams::Status stat;
        extraInfo.clear();
        if (!getParsedData()->streamTo(&extraInfo,stat))
            return Memory::Streams::Parsing::PARSE_STAT_ERROR;
        curProcVal = E_REQ_METHOD_AUTH;
    }break;
    case E_REQ_METHOD_AUTH:
    {
        if (getParsedData()->toString().empty())
        {
            // Finished...
            curProcVal = E_REQ_METHOD_NAME;
            curProcVal = E_REQ_METHOD_REQID;
        }
        else
        {
            // Authentication parsing...
            Memory::Streams::Status stat;
            Memory::Streams::JSON_Streamable s;
            Authentication auth;

            if (!getParsedData()->streamTo(&s,stat))
            {
                // std::cout << "failed to parse from data to json :S" << std::endl << std::flush;
                return Memory::Streams::Parsing::PARSE_STAT_ERROR;
            }
            if (!auth.fromJSON((*s.getValue())))
            {
                // std::cout << "failed to parse from json" << std::endl << std::flush;
                return Memory::Streams::Parsing::PARSE_STAT_ERROR;
            }

            //std::cout << "OK parsed." << std::endl << std::flush;

            addAuthentication(auth);
        }
    }break;
    case E_REQ_METHOD_REQID:
    {
        reqId = stoull(getParsedData()->toString());
        curProcVal = E_REQ_METHOD_SESSIONID;
    }break;
    case E_REQ_METHOD_SESSIONID:
    {
        sessionID = getParsedData()->toString();
        curProcVal = E_REQ_METHOD_RETCODE;
    }break;
    case E_REQ_METHOD_RETCODE:
    {
        retcode = stoi(getParsedData()->toString());
        return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
    }break;
    }
    return Memory::Streams::Parsing::PARSE_STAT_GET_MORE_DATA;
}

std::string Request::getDomainName() const
{
    return domainName;
}

void Request::setDomainName(const std::string &value)
{
    domainName = value;
}

std::string Request::getUserName() const
{
    return userName;
}

void Request::setUserName(const std::string &value)
{
    userName = value;
}

std::string Request::getSessionID() const
{
    return sessionID;
}

void Request::setSessionID(const std::string &value)
{
    sessionID = value;
}

int Request::getRetCode()
{
    return retcode;
}

void Request::setRetCode(int value)
{
    retcode = value;
}

uint64_t Request::getReqId()
{
    return reqId;
}

void Request::setReqId(const uint64_t &value)
{
    reqId = value;
}

std::set<uint32_t> Request::getAuthenticationsIdxs()
{
    std::set<uint32_t> r;
    for (const auto & i : authentications)
        r.insert(i.first);
    return r;
}

Authentication Request::getAuthentication(const uint32_t &idx)
{
    if (authentications.find(idx) != authentications.end())
        return authentications[idx];

    Authentication r;
    return r;
}

bool Request::stream(Memory::Streams::Status &wrStat)
{
    /*std::cout << "Writting into channel: -------------------------------------" << std::endl << std::flush;
    print();
    std::cout << "------------------------------------------------------------" << std::endl << std::flush;*/

    if (!upStream->writeString(methodName,wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!upStream->writeString(rpcMode,wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!payload.streamTo(upStream,wrStat)) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!ids.streamTo(upStream,wrStat)) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!extraInfo.streamTo(upStream,wrStat)) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    for (const auto & i : authentications)
    {
        Memory::Streams::JSON_Streamable s;

        s.setValue(i.second.toJSON());

        if (!s.streamTo(upStream,wrStat)) return false;
        if (!upStream->writeString("\n",wrStat).succeed) return false;
    }
    if (!upStream->writeString("\n",wrStat).succeed) return false;


    if (!upStream->writeString(std::to_string(reqId),wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!upStream->writeString(sessionID,wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    if (!upStream->writeString(std::to_string(retcode),wrStat).succeed) return false;
    if (!upStream->writeString("\n",wrStat).succeed) return false;

    return true;
}

Json::Value Request::toJSON()
{
    Json::Value r;
    r["method"] = methodName;
    r["mode"] = rpcMode;
    r["payload"] = *payload.getValue();
    r["ids"] = *ids.getValue();
    r["extraInfo"] = *extraInfo.getValue();
    r["reqId"] = (Json::UInt64)reqId;
    r["retCode"] = retcode;
    r["sessionId"] = sessionID;

    for (const auto & i : authentications)
    {
        r["auth"][std::to_string(i.first)] = i.second.toJSON();
    }

    return r;
}

void Request::print()
{
    std::cout << "REQ> Executing method: " << methodName << std::endl << std::flush;
    std::cout << "REQ> On rpc mode     : " << rpcMode << std::endl << std::flush;
    std::cout << "REQ> With payload    : " << payload.getString() <<std::flush;
    std::cout << "REQ> With id's       : " << ids.getString() <<  std::flush;
    std::cout << "REQ> With extraInfo  : " << extraInfo.getString() <<  std::flush;

    for (const auto & i : authentications)
    {
        Memory::Streams::JSON_Streamable s;
        s.setValue(i.second.toJSON());
        std::cout << ">>>> With auth: " << s.getString() << std::endl << std::flush;
    }
}

bool Request::setExtraInfo(const std::string &extraInfo)
{
    if (extraInfo.empty()) return true;
    Json::Reader reader;
    return reader.parse(extraInfo, (*this->extraInfo.getValue()));
}

bool Request::setPayload(const std::string &payload)
{
    if (payload.empty()) return true;
    Json::Reader reader;
    return reader.parse(payload, (*this->payload.getValue()));
}

bool Request::setAuthentications(const std::string &sAuthentications)
{
    if (sAuthentications.empty()) return true;

    Json::Value jAuthentications;
    Json::Reader reader;
    if (!reader.parse(sAuthentications, jAuthentications)) return false;
    if (!jAuthentications.isObject()) return false;

    if (jAuthentications.isObject())
    {
        for (auto idx : jAuthentications.getMemberNames())
        {
            if ( jAuthentications[idx].isMember("pass") )
            {
                addAuthentication(std::stoul(idx), jAuthentications[idx]["pass"].asString());
            }
        }
    }

    return true;
}

void Request::setExtraInfo(const Json::Value &extraInfo)
{
    (*this->extraInfo.getValue()) = extraInfo;
}

void Request::setPayload(const Json::Value &payload)
{
    (*this->payload.getValue()) = payload;
}

Json::Value Request::getPayload()
{
    return (*this->payload.getValue());
}

Json::Value Request::getExtraInfo()
{
    return (*this->extraInfo.getValue());
}

void Request::setMethodName(const std::string &value)
{
    methodName = value;
}

std::string Request::getMethodName() const
{
    return methodName;
}

std::string Request::getRpcMode() const
{
    return rpcMode;
}

void Request::setRpcMode(const std::string &value)
{
    rpcMode = value;
}

void Request::clear()
{
    retcode = 0;
    sessionID = "";
    curProcVal = E_REQ_METHOD_NAME;
    rpcMode = "EXEC";
    methodName.clear();
    payload.clear();
    ids.clear();
    extraInfo.clear();
    authentications.clear();
    setReqId(0);
}

void Request::addAuthentication(const Authentication &auth)
{
    authentications[auth.getPassIndex()] = auth;
}

void Request::addAuthentication(uint32_t passIndex,const std::string &pass)
{
    authentications[passIndex].setPassIndex(passIndex);
    authentications[passIndex].setUserPass(pass);
}
