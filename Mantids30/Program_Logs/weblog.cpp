#include "weblog.h"

#include <inttypes.h>

using namespace Mantids30::Program::Logs;

WebLog::WebLog(unsigned int _logMode) : LogBase(_logMode)
{}

void WebLog::log(
    const std::string &method, const std::string &url, int statusCode, const std::string &userAgent, const std::string &clientIP)
{
    FILE *fp = stdout;

    fprintf(fp, "W/");
    if (enableDateLogging)
    {
        printDate(fp);
    }

    if (enableColorLogging)
    {
        switch (statusCode / 100)
        {
        case 2:
            printColorBold(fp, getAlignedValue("INFO", 12).c_str());
            break;
        case 3:
            printColorBlue(fp, getAlignedValue("REDIRECT", 12).c_str());
            break;
        case 4:
            printColorRed(fp, getAlignedValue("CLIENT_ERROR", 12).c_str());
            break;
        case 5:
            printColorPurple(fp, getAlignedValue("SERVER_ERROR", 12).c_str());
            break;
        default:
            fprintf(fp, "%s", getAlignedValue("UNKNOWN", 12).c_str());
        }
    }
    else
    {
        switch (statusCode / 100)
        {
        case 2:
            fprintf(fp, "%s", getAlignedValue("INFO", 12).c_str());
            break;
        case 3:
            fprintf(fp, "%s", getAlignedValue("REDIRECT", 12).c_str());
            break;
        case 4:
            fprintf(fp, "%s", getAlignedValue("CLIENT_ERROR", 12).c_str());
            break;
        case 5:
            fprintf(fp, "%s", getAlignedValue("SERVER_ERROR", 12).c_str());
            break;
        default:
            fprintf(fp, "%s", getAlignedValue("UNKNOWN", 12).c_str());
        }
    }

    fprintf(fp, " METHOD=\"%s\" URL=\"%s\" STATUS=%" PRIi32 " USER_AGENT=\"%s\" CLIENT_IP=\"%s\"\n", method.c_str(), url.c_str(), static_cast<int32_t>(statusCode), userAgent.c_str(), clientIP.c_str());

    fflush(stdout);
}
