#pragma once

#include <string>
#include <ctime>


#include "logbase.h"


namespace Mantids30 { namespace Program { namespace Logs {

class WebLog : public LogBase
{
public:
    WebLog(unsigned int _logMode = MODE_STANDARD);

    void log(const std::string& method,
                    const std::string& url,
                    int statusCode,
                    const std::string& userAgent,
                    const std::string& clientIP);


};


}}}



