#include "httpv1_server.h"

using namespace Mantids30::Network::Protocol;

using namespace std;

std::string HTTP::HTTPv1_Server::htmlEncode(const std::string &rawStr)
{
    std::string output;
    output.reserve(rawStr.size());
    for (const char &i : rawStr)
    {
        switch (i)
        {
        case '<':
            output.append("&lt;");
            break;
        case '>':
            output.append("&gt;");
            break;
        case '\"':
            output.append("&quot;");
            break;
        case '&':
            output.append("&amp;");
            break;
        case '\'':
            output.append("&apos;");
            break;
        default:
            output.append(&i, 1);
            break;
        }
    }
    return output;
}
