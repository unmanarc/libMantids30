#ifndef FULLREQUEST_H
#define FULLREQUEST_H

#include <mdz_mem_vars/vars.h>
#include "req_cookies.h"
#include "req_requestline.h"
#include "common_content.h"

namespace Mantids { namespace Protocols { namespace HTTP { namespace Request {


struct DataObjects
{
    // Host Information:
    const char * CLIENT_IP;
    std::string * VIRTUAL_HOST;
    uint16_t * VIRTUAL_PORT;

    // Authentication information
    std::string AUTH_USER,AUTH_PASS;
    bool USING_BASIC_AUTH;

    // User Agent
    std::string USER_AGENT;

    // Proceced information:
    Memory::Abstract::Vars *VARS_GET, *VARS_POST;
    MIME::MIME_HeaderOption * VARS_COOKIES;

    // Original Values:
    MIME::MIME_Sub_Header * clientHeaders; // User agent, Host, etc ...
    Request::RequestLine * clientRequestLine; // method type, and other options...
    Common::Content * clientContentData; // content data...
};

}}}}



#endif // FULLREQUEST_H
