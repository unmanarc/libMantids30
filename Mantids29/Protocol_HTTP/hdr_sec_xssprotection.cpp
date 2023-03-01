#include "hdr_sec_xssprotection.h"
#include <stdexcept>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace Mantids29::Protocols::HTTP::Headers::Security;
using namespace Mantids29::Protocols::HTTP;
using namespace Mantids29;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

XSSProtection::XSSProtection()
{
    setDefaults();
}

string XSSProtection::toString()
{
    if (!m_parameterActivated)
    {
        return "0";
    }
    else
    {
        string r = "1";
        if (m_modeBlocking)
            r+= "; mode=block";
        if (!m_reportURL.empty())
            r+= "; report=" + m_reportURL;
        return r;
    }
}

bool XSSProtection::fromString(const string &sValue)
{
    vector<string> parts;
    split(parts,sValue,is_any_of("; "),token_compress_on);

    setDefaults();

    if (parts.empty())
    {
        m_parameterActivated = false;
    }
    else if (parts.size() == 1)
    {
        m_parameterActivated = parts[0]=="1";
    }
    else if (parts.size() >= 2)
    {
        m_parameterActivated = parts[0]=="1";
        if (m_parameterActivated)
        {
            m_modeBlocking = false;
            for ( size_t i=1; i<parts.size();i++ )
            {
                if (iequals(parts[i],"mode=block"))
                    m_modeBlocking = true;
                else if (istarts_with(parts[i],"report="))
                    m_reportURL = parts[i].substr(7);
            }
        }
    }
    return true;
}

void XSSProtection::setDefaults()
{
    m_parameterActivated = true;
    m_modeBlocking = true;
    m_reportURL = "";
}
