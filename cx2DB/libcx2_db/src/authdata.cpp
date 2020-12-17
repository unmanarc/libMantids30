#include "authdata.h"

using namespace CX2::Database;

AuthData::AuthData()
{

}

std::string AuthData::getPass() const
{
    return pass;
}

void AuthData::setPass(const std::string &value)
{
    pass = value;
}

std::string AuthData::getUser() const
{
    return user;
}

void AuthData::setUser(const std::string &value)
{
    user = value;
}
