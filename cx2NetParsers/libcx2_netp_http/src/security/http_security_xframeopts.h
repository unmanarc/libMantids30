#ifndef HTTP_SECURITY_XFRAMEOPTS_H
#define HTTP_SECURITY_XFRAMEOPTS_H

#include <string>

namespace CX2 { namespace Network { namespace HTTP {

enum eHTTP_Security_XFrameOptsValues {
    HTTP_XFRAME_NONE=0,
    HTTP_XFRAME_DENY=1,
    HTTP_XFRAME_SAMEORIGIN=2,
    HTTP_XFRAME_ALLOWFROM=3
};

class HTTP_Security_XFrameOpts
{
public:
    HTTP_Security_XFrameOpts();
    HTTP_Security_XFrameOpts(const eHTTP_Security_XFrameOptsValues & value, const std::string & fromURL);

    bool isNotActivated() const;

    std::string toValue();
    bool fromValue(const std::string & sValue);

    std::string getFromURL() const;
    void setFromURL(const std::string &value);

    eHTTP_Security_XFrameOptsValues getValue() const;

private:
    eHTTP_Security_XFrameOptsValues value;
    std::string fromURL;
};

}}}

#endif // HTTP_SECURITY_XFRAMEOPTS_H
