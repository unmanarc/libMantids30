#ifndef IAUTH_VALIDATION_ACCOUNT_H
#define IAUTH_VALIDATION_ACCOUNT_H

#include <string>
#include "ds_auth_modes.h"
#include "ds_auth_reasons.h"
#include "ds_auth_passmodes.h"
#include "ds_auth_passworddata.h"

#include <cx2_thr_safecontainers/map_element.h>

namespace CX2 { namespace Authorization { namespace Validation {

class IAuth_Validation_Account : public CX2::Threads::Safe::Map_Element
{
public:
    IAuth_Validation_Account();
    virtual ~IAuth_Validation_Account();

    static DataStructs::sPasswordData genPassword(const std::string & passwordInput, const DataStructs::PasswordModes & passMode, bool forceExpiration = false, const time_t &expirationDate = std::numeric_limits<time_t>::max(), uint32_t _2faSteps = 0);

    virtual std::string accountConfirmationToken(const std::string & accountName)=0;
    virtual DataStructs::sPasswordBasicData accountPasswordBasicData(const std::string & accountName, bool * found, uint32_t passIndex=0)=0;
    virtual DataStructs::AuthReason authenticate(const std::string & accountName, const std::string & password, uint32_t passIndex = 0, DataStructs::AuthMode authMode = DataStructs::AUTH_MODE_PLAIN, const std::string & cramSalt = "")=0;

    virtual bool accountValidateAttribute(const std::string & accountName, const std::string & attribName)=0;

protected:
    DataStructs::AuthReason validatePassword(const DataStructs::sPasswordData & storedPassword, const std::string & passwordInput, const std::string &cramSalt, DataStructs::AuthMode authMode);

private:


    DataStructs::AuthReason validateCRAM(const std::string & passwordFromDB, const std::string & passwordInput, const std::string &cramSalt);
    DataStructs::AuthReason validateGAuth(const std::string & seed, const std::string & token);
};

}}}

#endif // IAUTH_VALIDATION_ACCOUNT_H
