#ifndef HTMLIENGINE_H
#define HTMLIENGINE_H

#include <Mantids29/Protocol_HTTP/httpv1_base.h>
#include <Mantids29/Helpers/json.h>

#include "apiclienthandler.h"

namespace Mantids29 { namespace Network { namespace Servers { namespace Web {

class HTMLIEngine
{
public:
    HTMLIEngine();
    static Protocols::HTTP::Status::eRetCode processResourceFile( APIClientHandler * clientHandler, const std::string & sRealFullPath );

private:
    static void replaceTagByJVar( std::string & content, const std::string & tag, const json & value, bool replaceFirst = false, const std::string & varName = "" );
    static void replaceHexCodes( std::string &content );
};

}}}}

#endif // HTMLIENGINE_H
