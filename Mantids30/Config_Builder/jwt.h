#pragma once

#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Program_Logs/applog.h>
#include <boost/property_tree/ptree.hpp>
#include <memory>

namespace Mantids30 {
namespace Program {
namespace Config {


class JWT
{
public:
    JWT() = default;

    static std::shared_ptr<DataFormat::JWT> createJWTSigner(Mantids30::Program::Logs::AppLog *log,
                                                                       boost::property_tree::ptree *ptr,
                                                                       const std::string &configClassName);

    static std::shared_ptr<DataFormat::JWT> createJWTValidator(Mantids30::Program::Logs::AppLog *log,
                                                                          boost::property_tree::ptree *ptr,
                                                                          const std::string &configClassName);

    static std::shared_ptr<DataFormat::JWT> createJWTValidator(Mantids30::Program::Logs::AppLog *log,
                                                                          const std::string & algorithm,
                                                                          const std::string & key);
private:
    static bool createHMACSecret(Program::Logs::AppLog *log,const std::string &filePath);
    static bool createRSASecret(Program::Logs::AppLog *log, const std::string &keyPath, const std::string &crtPath, uint16_t keySize = 4096);
};

}
} // namespace Config
} // namespace Mantids30
