#ifndef HTTP_SECURITY_HSTS_H
#define HTTP_SECURITY_HSTS_H

#include <string>
namespace Mantids { namespace Protocols { namespace HTTP { namespace Headers { namespace Security {

class HSTS
{
public:
    HSTS();
    HSTS(uint32_t maxAge, bool includeSubDomains = false, bool preload = false);

    void setDefaults();

    bool getActivated() const;
    void setActivated(bool value);

    /**
     * @brief getPreload Get if using preload option
     * @return
     */
    bool getPreload() const;
    /**
     * @brief setPreload Preload will include this domain in the preload list (see https://hstspreload.org/)
     * @param value
     */
    void setPreload(bool value);

    bool getIncludeSubDomains() const;
    void setIncludeSubDomains(bool value);

    std::string toValue();
    bool fromValue(const std::string & sValue);

private:
    bool activated,preload,includeSubDomains;
    uint32_t maxAge;
};

}}}}}
#endif // HTTP_SECURITY_HSTS_H

