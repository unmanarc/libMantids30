#ifndef FULLRESPONSE_H
#define FULLRESPONSE_H

#include "rsp_cookies.h"
#include "common_content.h"
#include "rsp_status.h"

#include "hdr_sec_xframeopts.h"
#include "hdr_sec_xssprotection.h"
#include "hdr_sec_hsts.h"
#include "hdr_cachecontrol.h"

namespace Mantids { namespace Network { namespace HTTP { namespace Response {

struct DataObject
{
    // Proceced information:
    Response::Cookies_ServerSide * setCookies;

    // Security Parameters:
    Headers::Security::XFrameOpts * secXFrameOpts;
    Headers::Security::XSSProtection * secXSSProtection;
    Headers::Security::HSTS * secHSTS;
    Headers::CacheControl * cacheControl;

    // Original Values:
    MIME::MIME_Sub_Header * headers; //
    Response::Status * status; //
    Common::Content * contentData; //

    std::string *authenticate;
    std::string * contentType;
    bool bNoSniff;
};

}}}}


#endif // FULLRESPONSE_H
