#pragma once

#include <Mantids30/Runtime/Logs/applog.h>
#include <boost/property_tree/ptree.hpp>

#include <optional>

namespace Mantids30::Runtime::Service::Config {
class Loader
{
public:
    Loader() = default;

    static std::optional<boost::property_tree::ptree> loadSecureApplicationConfig(Mantids30::Runtime::Logs::AppLog *log, const std::string &dir, const std::string &filePath);
};

} // namespace Mantids30::Runtime::Service::Config
