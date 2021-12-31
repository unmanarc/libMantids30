#ifndef AUTHINFO_H
#define AUTHINFO_H

#include <string>

namespace Mantids { namespace Database {

class AuthData
{
public:
    AuthData();

    std::string getPass() const;
    void setPass(const std::string &value);

    std::string getUser() const;
    void setUser(const std::string &value);

private:
    std::string user,pass;
};
}}

#endif // AUTHINFO_H
