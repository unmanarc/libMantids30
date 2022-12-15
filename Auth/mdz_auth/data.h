#ifndef DATA_H
#define DATA_H

#include <string>
#include <mdz_hlp_functions/json.h>
namespace Mantids { namespace Authentication {

/**
 * @brief The Authentication class
 */
class Data
{
public:
    Data();
    Data(const std::string & pass, const uint32_t & idx);

    bool fromString(const std::string & sAuth);
    bool fromJSON( const json & x );
    json toJSON() const;

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

#endif // DATA_H
