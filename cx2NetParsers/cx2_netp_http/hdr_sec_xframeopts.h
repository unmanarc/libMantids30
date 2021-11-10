#ifndef HTTP_SECURITY_XFRAMEOPTS_H
#define HTTP_SECURITY_XFRAMEOPTS_H

#include <string>

namespace CX2 { namespace Network { namespace HTTP { namespace Headers { namespace Security {

enum eXFrameOptsValues {
    HTTP_XFRAME_NONE=0,
    HTTP_XFRAME_DENY=1,
    HTTP_XFRAME_SAMEORIGIN=2,
    HTTP_XFRAME_ALLOWFROM=3
};

class XFrameOpts
{
public:
    XFrameOpts();
    XFrameOpts(const eXFrameOptsValues & value, const std::string & fromURL);

    void setDefaults();

    bool isNotActivated() const;

    std::string toValue();
    bool fromValue(const std::string & sValue);

    std::string getFromURL() const;
    void setFromURL(const std::string &value);

    eXFrameOptsValues getValue() const;

private:
    eXFrameOptsValues value;
    std::string fromURL;
};

}}}}}

#endif // HTTP_SECURITY_XFRAMEOPTS_H
