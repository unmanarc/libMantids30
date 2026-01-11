#pragma once

#include <Mantids30/Program_Logs/applog.h>
#include <boost/property_tree/ptree.hpp>

#include <optional>

namespace Mantids30::Program {
namespace Config {
class Loader
{
public:
    Loader() = default;

    static std::optional<boost::property_tree::ptree> loadSecureApplicationConfig(Mantids30::Program::Logs::AppLog *log, const std::string &dir, const std::string &filePath);
};

} // namespace Config
} // namespace Mantids30::Program
