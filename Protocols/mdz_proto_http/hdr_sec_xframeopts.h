#ifndef HTTP_SECURITY_XFRAMEOPTS_H
#define HTTP_SECURITY_XFRAMEOPTS_H

#include <string>

namespace Mantids { namespace Protocols { namespace HTTP { namespace Headers { namespace Security {

class XFrameOpts
{
public:
    enum eOptsValues {
        HTTP_XFRAME_NONE=0,
        HTTP_XFRAME_DENY=1,
        HTTP_XFRAME_SAMEORIGIN=2,
        HTTP_XFRAME_ALLOWFROM=3
    };

    XFrameOpts();
    XFrameOpts(const eOptsValues & value, const std::string & fromURL);

    void setDefaults();

    bool isNotActivated() const;

    std::string toValue();
    bool fromValue(const std::string & sValue);

    std::string getFromURL() const;
    void setFromURL(const std::string &value);

    eOptsValues getValue() const;

private:
    eOptsValues value;
    std::string fromURL;
};

}}}}}

#endif // HTTP_SECURITY_XFRAMEOPTS_H
