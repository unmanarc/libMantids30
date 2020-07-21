#ifndef HLP_ENCODERS_H
#define HLP_ENCODERS_H

#include <string>

namespace CX2 { namespace Helpers {

class Encoders
{
public:
    Encoders();
    // B64 Encoding
    static std::string fromBase64(std::string const& sB64Buf);
    static std::string toBase64(char const* buf, uint32_t count);

    // URL Percent Encoding
    static std::string toURL(const std::string &str);
    static std::string fromURL(const std::string &urlEncodedStr);

    // Hex Encoding
    static std::string toHex(const unsigned char *data, size_t len);
    static void fromHex(const std::string &hexValue, unsigned char *data, size_t maxlen);

    // Hex Helpers
    static char toHexPair(char value, char part);
    static bool isHexChar(char v);
    static char hexToValue(char v);
private:
    static bool mustBeEncoded(char c);
    static size_t calcExpandedStringSize(const std::string &str);
    static const std::string b64Chars;

};

}}

#endif // HLP_ENCODERS_H
