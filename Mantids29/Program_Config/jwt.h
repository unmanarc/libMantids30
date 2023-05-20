#pragma once

#include <Mantids29/DataFormat_JWT/jwt.h>
#include <Mantids29/Program_Logs/applog.h>
#include <boost/property_tree/ptree.hpp>
#include <memory>

namespace Mantids29 {
namespace Config {

class JWT
{
public:
    JWT() = default;

    static std::shared_ptr<Mantids29::DataFormat::JWT> createJWTSigner(Mantids29::Program::Logs::AppLog *log,
                                                                       boost::property_tree::ptree *ptr,
                                                                       const std::string &configClassName);

    static std::shared_ptr<Mantids29::DataFormat::JWT> createJWTValidator(Mantids29::Program::Logs::AppLog *log,
                                                                          boost::property_tree::ptree *ptr,
                                                                          const std::string &configClassName);

private:
    static bool createHMACSecret(Program::Logs::AppLog *log,const std::string &filePath);
    static bool createRSASecret(Program::Logs::AppLog *log, const std::string &keyPath, const std::string &crtPath, uint16_t keySize = 4096);
};

} // namespace Config
} // namespace Mantids29
