#ifndef REQUEST_H
#define REQUEST_H

#include <json/json.h>
#include <set>
#include <cx2_mem_vars/substreamparser.h>
#include "json_streamable.h"
#include "authentication.h"

namespace CX2 { namespace RPC {

class Request
{
public:
    Request();
//    Json::Value toJSON();

    void print();

//    bool setExtraInfo(const std::string & extraInfo);
//    bool setPayload(const std::string & payload);
    bool setAuthentications(const std::string & sAuthentications);

 //   void setExtraInfo(const Json::Value & extraInfo);
    //void setPayload(const Json::Value & payload);

//    Json::Value getPayload();

    void clear();

    void addAuthentication(const Authentication & auth);
    void addAuthentication(uint32_t passIndex, const std::string &pass);

    std::set<uint32_t> getAuthenticationsIdxs();
    Authentication getAuthentication( const uint32_t & idx );

/*    uint64_t getReqId();
    void setReqId(const uint64_t &value);

    int getRetCode();
    void setRetCode(int value);

*/
/*    std::string getUserName() const;
    void setUserName(const std::string &value);

    std::string getDomainName() const;
    void setDomainName(const std::string &value);
*/

private:
    //Memory::Streams::JSON_Streamable payload;
   // std::string userName, domainName; // methodName, rpcMode
  //  uint64_t reqId;

    //int retcode;
    std::map<uint32_t,Authentication> authentications;
};

}}
#endif // REQUEST_H
