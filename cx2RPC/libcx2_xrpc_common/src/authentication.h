#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <string>
#include <json/json.h>
namespace CX2 { namespace RPC {

class Authentication
{
public:
    Authentication();
    Authentication(const std::string & pass, const uint32_t & idx);

    bool fromJSON( const Json::Value & x );
    Json::Value toJSON() const;

    std::string getUserPass() const;
    void setUserPass(const std::string &value);

    uint32_t getPassIndex() const;
    void setPassIndex(const uint32_t &value);

private:
    std::string userName,userPass, domainName;
    uint32_t passIndex;
};
}}

#endif // AUTHENTICATION_H
