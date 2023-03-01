#ifndef REQUEST_H
#define REQUEST_H

#include <Mantids29/Helpers/json.h>
#include <set>
//#include <Mantids29/Memory/subparser.h>
//#include "streamablejson.h"
#include "data.h"

namespace Mantids29 { namespace Authentication {

class Multi
{
public:
    Multi();

    /**
     * @brief setAuthentications Set the authentication string.
     * @param sAuthentications string in JSON Format.
     * @return if the string have been correctly parsed, returns true, else false.
     */
    bool setAuthentications(const std::string & sAuthentications);
    /**
     * @brief Deserializes multiple authentications from a JSON object.
     * @param jsonObject The JSON object containing the multi authentications.
     * @return true if the deserialization was successful, false otherwise.
     */
    bool setJson(const json & auths);
    /**
     * @brief clear Clear authentications
     */
    void clear();
    /**
     * @brief addAuthentication Manually add an authentication
     * @param auth Authentication object.
     */
    void addAuthentication(const Data & auth);
    /**
     * @brief addAuthentication Add an authentication as passwordIndex+Secret
     * @param passwordIndex Authentication Secret Index
     * @param password Secret
     */
    void addAuthentication(uint32_t passwordIndex, const std::string &password);
    /**
     * @brief getAvailableIndices Get authentications Indexes.
     * @return set of password indexes.
     */
    std::set<uint32_t> getAvailableIndices();
    /**
     * @brief getAuthentication Get authentication object given a password index.
     * @param idx Authentication Secret Index.
     * @return Authentication Object.
     */
    Data getAuthentication( const uint32_t & passwordIndex );


private:
    std::map<uint32_t,Data> m_authentications;
};

}}
#endif // REQUEST_H
