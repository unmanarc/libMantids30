#pragma once

#include <memory>
#include <Mantids29/DataFormat_JWT/jwt.h>
#include <Mantids29/Program_Logs/applog.h>
#include <boost/property_tree/ptree.hpp>

namespace Mantids29 { namespace Config {

class JWT
{
public:
    JWT() = default;

    static std::shared_ptr<Mantids29::DataFormat::JWT> createJWTSigner( Mantids29::Program::Logs::AppLog * log,
                                                                        boost::property_tree::ptree * ptr,
                                                                        const std::string & configClassName);

    static std::shared_ptr<Mantids29::DataFormat::JWT> createJWTValidator(Mantids29::Program::Logs::AppLog * log,
                                                                          boost::property_tree::ptree * ptr,
                                                                          const std::string & configClassName);


};

}}

