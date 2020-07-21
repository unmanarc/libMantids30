#ifndef REQUEST_H
#define REQUEST_H

#include <json/json.h>
#include <set>
#include <cx2_mem_streamparser/substreamparser.h>
#include "json_streamable.h"
#include "authentication.h"

namespace CX2 { namespace RPC { namespace XRPC {

enum eReqProcVal {
    E_REQ_METHOD_NAME=0,
    E_REQ_METHOD_RPCMODE=1,
    E_REQ_METHOD_PAYLOAD=2,
    E_REQ_METHOD_EXTRAINFO=3,
    E_REQ_METHOD_IDS=4,
    E_REQ_METHOD_AUTH=5,
    E_REQ_METHOD_REQID=6,
    E_REQ_METHOD_SESSIONID=7,
    E_REQ_METHOD_RETCODE=8
};

class Request : public Memory::Streams::Parsing::SubParser
{
public:
    Request();
    bool stream(Memory::Streams::Status & wrStat) override;
    Json::Value toJSON();

    void print();

    bool setExtraInfo(const std::string & extraInfo);
    bool setPayload(const std::string & payload);
    bool setAuthentications(const std::string & sAuthentications);

    void setExtraInfo(const Json::Value & extraInfo);
    void setPayload(const Json::Value & payload);

    Json::Value getPayload();
    Json::Value getExtraInfo();

    void setMethodName(const std::string &value);
    std::string getMethodName() const;

    std::string getRpcMode() const;
    void setRpcMode(const std::string &value);

    void clear();

    void addAuthentication(const Authentication & auth);
    void addAuthentication(uint32_t passIndex, const std::string &pass);

    std::set<uint32_t> getAuthenticationsIdxs();
    Authentication getAuthentication( const uint32_t & idx );

    uint64_t getReqId();
    void setReqId(const uint64_t &value);

    int getRetCode();
    void setRetCode(int value);

    std::string getSessionID() const;
    void setSessionID(const std::string &value);

    std::string getUserName() const;
    void setUserName(const std::string &value);

    std::string getDomainName() const;
    void setDomainName(const std::string &value);

protected:
    Memory::Streams::Parsing::ParseStatus parse() override;

private:
    Memory::Streams::JSON_Streamable payload, extraInfo, ids;
    std::string methodName, rpcMode, sessionID, userName, domainName;
    uint64_t reqId;

    int retcode;
    std::map<uint32_t,Authentication> authentications;
    eReqProcVal curProcVal;
};

}}}
#endif // REQUEST_H
