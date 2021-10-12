#ifndef FULLRESPONSE_H
#define FULLRESPONSE_H

#include "http_cookies_serverside.h"
#include "http_content.h"
#include "http_status.h"

#include "http_security_xframeopts.h"
#include "http_security_xssprotection.h"
#include "http_security_hsts.h"

namespace CX2 { namespace Network { namespace HTTP {

struct sHTTP_ResponseData
{
    // Proceced information:
    HTTP_Cookies_ServerSide * setCookies;

    // Security Parameters:
    HTTP_Security_XFrameOpts * secXFrameOpts;
    HTTP_Security_XSSProtection * secXSSProtection;
    HTTP_Security_HSTS * secHSTS;

    // Original Values:
    MIME::MIME_Sub_Header * headers; //
    HTTP_Status * status; //
    HTTP_Content * contentData; //

    std::string contentType;
    bool bNoSniff;
};

}}}

#endif // FULLRESPONSE_H
