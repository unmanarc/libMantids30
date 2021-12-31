#ifndef HTTP_SECURITY_XSSPROTECTION_H
#define HTTP_SECURITY_XSSPROTECTION_H

#include <string>

namespace Mantids { namespace Network { namespace HTTP { namespace Headers { namespace Security {

class XSSProtection
{
public:
    XSSProtection();
    XSSProtection(bool activated, bool blocking);
    XSSProtection(bool activated, const std::string &reportURL);

    bool getActivated() const;
    void setActivated(bool value);

    bool getBlocking() const;
    void setBlocking(bool value);

    std::string getReportURL() const;
    void setReportURL(const std::string &value);

    std::string toValue();
    bool fromValue(const std::string & sValue);


    void setDefaults();

private:
    bool activated;
    bool blocking;
    std::string reportURL;
};

}}}}}

#endif // HTTP_SECURITY_XSSPROTECTION_H
