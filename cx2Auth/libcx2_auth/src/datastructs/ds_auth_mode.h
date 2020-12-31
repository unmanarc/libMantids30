#ifndef AUTH_MODES_H
#define AUTH_MODES_H

#include <string>

namespace CX2 { namespace Authentication  {

enum Mode
{
    MODE_PLAIN,   // NOT RECOMMENDED.
    MODE_CRAM
};

static Mode getAuthModeFromString( const std::string & mode )
{
    if (mode == "CRAM")
        return MODE_CRAM;
    else
        return MODE_PLAIN;
}

static std::string getStringFromAuthMode( const Mode & mode )
{
    switch (mode)
    {
    case MODE_PLAIN:
        return "PLAIN";
    case MODE_CRAM:
        return "CRAM";
    };
}



}}



#endif // AUTH_MODES_H
