#ifndef HTTP_SECURITY_XSSPROTECTION_H
#define HTTP_SECURITY_XSSPROTECTION_H

#include <string>

namespace CX2 { namespace Network { namespace HTTP {

class HTTP_Security_XSSProtection
{
public:
    HTTP_Security_XSSProtection();
    HTTP_Security_XSSProtection(bool activated, bool blocking);
    HTTP_Security_XSSProtection(bool activated, const std::string &reportURL);

    bool getActivated() const;
    void setActivated(bool value);

    bool getBlocking() const;
    void setBlocking(bool value);

    std::string getReportURL() const;
    void setReportURL(const std::string &value);

    std::string toValue();
    bool fromValue(const std::string & sValue);


private:
    bool activated;
    bool blocking;
    std::string reportURL;
};

}}}

#endif // HTTP_SECURITY_XSSPROTECTION_H
