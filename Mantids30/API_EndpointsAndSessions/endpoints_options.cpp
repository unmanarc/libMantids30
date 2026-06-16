#include "endpoints_options.h"
#include "Mantids30/Protocol_HTTP/api_return.h"

using namespace Mantids30::API;

void Endpoints_Options::setEndpointOptions(const std::string &endpointPath, const OptionsHandlerConfig &config)
{
    if (endpointPath.empty())
    {
        // Global configuration
        m_globalOptionsConfig = config;
        m_optionsEnabled = true;
    }
    else
    {
        // Per-endpoint configuration
        m_perEndpointOptionsConfig[endpointPath] = config;
        m_optionsEnabled = true;
    }
}

const OptionsHandlerConfig *Endpoints_Options::getGlobalOptionsConfig() const
{
    return &m_globalOptionsConfig;
}

const OptionsHandlerConfig *Endpoints_Options::getOptionsConfigOnEndpoint(const std::string &endpointPath) const
{
    std::map<std::string, OptionsHandlerConfig>::const_iterator optIt = m_perEndpointOptionsConfig.find(endpointPath);
    if (optIt != m_perEndpointOptionsConfig.end())
    {
        return &optIt->second;
    }
    return nullptr;
}

Mantids30::API::APIReturn Endpoints_Options::buildCORSOptionsResponse(const Mantids30::API::OptionsHandlerConfig &config, const std::string &requestOrigin)
{
    Mantids30::API::APIReturn ret;
    ret.setStatus(Mantids30::Network::Protocol::HTTP::Status::Code::S_204_NO_CONTENT);
    config.configureAPIReturnOptionsHeaders(ret, requestOrigin);
    return ret;
}
