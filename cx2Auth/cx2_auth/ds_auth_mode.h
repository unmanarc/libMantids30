#ifndef AUTH_MODES_H
#define AUTH_MODES_H

#include <string>

namespace CX2 { namespace Authentication  {

enum Mode
{
    MODE_PLAIN,   // NOT RECOMMENDED.
    MODE_CHALLENGE
};

static Mode getAuthModeFromString( const std::string & mode )
{
    if (mode == "CHALLENGE")
        return MODE_CHALLENGE;
    else
        return MODE_PLAIN;
}

static std::string getStringFromAuthMode( const Mode & mode )
{
    switch (mode)
    {
    case MODE_PLAIN:
        return "PLAIN";
    case MODE_CHALLENGE:
        return "CHALLENGE";
    };
    return "PLAIN";
}



}}



#endif // AUTH_MODES_H
