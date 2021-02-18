#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <string>
#include <json/json.h>
namespace CX2 { namespace RPC {

/**
 * @brief The Authentication class
 */
class Authentication
{
public:
    Authentication();
    Authentication(const std::string & pass, const uint32_t & idx);

    bool fromString(const std::string & sAuth);
    bool fromJSON( const Json::Value & x );
    Json::Value toJSON() const;

    std::string getPassword() const;
    void setPassword(const std::string &value);

    uint32_t getPassIndex() const;
    void setPassIndex(const uint32_t &value);
/*
    std::string getUserName() const;
    void setUserName(const std::string &value);
*/
private:
    std::string /*userName,*/sPassword, domainName;
    uint32_t iPassIDX;
};
}}

#endif // AUTHENTICATION_H
