#pragma once

#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <Mantids30/Helpers/json.h>

#include "apiclienthandler.h"

namespace Mantids30 { namespace Network { namespace Servers { namespace Web {

class HTMLIEngine
{
public:
    static Protocols::HTTP::Status::Codes processResourceFile( APIClientHandler * clientHandler, const std::string & sRealFullPath );

private:
    static void replaceTagByJVar( std::string & content, const std::string & tag, const json & value, bool replaceFirst = false, const std::string & varName = "" );
};

}}}}

