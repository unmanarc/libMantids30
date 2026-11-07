#include "encoders.h"
#include "safeint.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <cinttypes>
#include <cstring>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <random>

using namespace std;
using namespace Mantids30::Helpers;

const std::string Encoders::m_b64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

string Encoders::decodeFromBase64Obf(std::string_view sB64Buf, const uint64_t &seed)
{
    unsigned char cont4[4], cont3[3];
    std::string decodedString;
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<char> dis;
    size_t count = sB64Buf.size(), x = 0, y = 0;
    int bufPos = 0;

    while (count-- && (sB64Buf[bufPos] != '=') && (isalnum(sB64Buf[bufPos]) || (sB64Buf[bufPos] == '/') || (sB64Buf[bufPos] == '+')))
    {
        cont4[x++] = sB64Buf[bufPos];
        bufPos++;
        if (x == 4)
        {
            for (x = 0; x < 4; x++)
            {
                cont4[x] = static_cast<unsigned char>(m_b64Chars.find(static_cast<char>(cont4[x])));
            }

            cont3[0] = (cont4[0] << 2) + ((cont4[1] & 0x30) >> 4);
            cont3[1] = ((cont4[1] & 0xf) << 4) + ((cont4[2] & 0x3c) >> 2);
            cont3[2] = ((cont4[2] & 0x3) << 6) + cont4[3];

            for (x = 0; (x < 3); x++)
            {
                decodedString += static_cast<char>(cont3[x] ^ static_cast<unsigned char>(dis(gen)));
            }
            x = 0;
        }
    }

    if (x)
    {
        for (y = x; y < 4; y++)
        {
            cont4[y] = 0;
        }
        for (y = 0; y < 4; y++)
        {
            cont4[y] = static_cast<unsigned char>(m_b64Chars.find(static_cast<char>(cont4[y])));
        }

        cont3[0] = (cont4[0] << 2) + ((cont4[1] & 0x30) >> 4);
        cont3[1] = ((cont4[1] & 0xf) << 4) + ((cont4[2] & 0x3c) >> 2);
        cont3[2] = ((cont4[2] & 0x3) << 6) + cont4[3];

        for (y = 0; (y < x - 1); y++)
        {
            decodedString += static_cast<char>(cont3[y] ^ static_cast<unsigned char>(dis(gen)));
        }
    }

    return decodedString;
}

string Encoders::encodeToBase64Obf(const unsigned char *buf, size_t count, const uint64_t &seed)
{
    std::string r;
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<char> dis;

    unsigned char *obfBuf = static_cast<unsigned char *>(malloc(count));
    if (!obfBuf)
    {
        return "";
    }

    for (size_t i = 0; i < count; i++)
    {
        obfBuf[i] = buf[i] ^ dis(gen);
    }

    r = encodeToBase64(obfBuf, count);
    free(obfBuf);
    return r;
}

string Encoders::encodeToBase64Obf(std::string_view buf, const uint64_t &seed)
{
    return encodeToBase64Obf(reinterpret_cast<const unsigned char *>(buf.data()), buf.size(), seed);
}

std::shared_ptr<Mem::BinaryDataContainer> Encoders::decodeFromBase64ToBin(string_view input, bool url)
{
    // Allocate a BinaryDataContainer object to store the decoded data
    std::shared_ptr<Mem::BinaryDataContainer> r = std::make_shared<Mem::BinaryDataContainer>(input.length() + 2);
    if (!r->data)
    {
        // Error handling: memory allocation failed
        return r;
    }

    // Make a mutable copy: base64 decoding (and the URL->std conversion below) needs a
    // std::string to point into. A single copy, same as the previous by-value signature.
    std::string inputCopy(input);
    const std::string *inputx = &inputCopy;
    std::string inputurl;

    if (url) // Check if url is not null
    {
        inputurl = input;   // Set inputurl to the input string
        inputx = &inputurl; // Set inputx to the address of inputurl

        // Check the length of the input URL mod 4
        switch (inputurl.length() % 4)
        {
        case 0: // If the length is 0 mod 4, do nothing
            break;
        case 2: // If the length is 2 mod 4, add == padding
            inputurl += "==";
            break;
        case 3: // If the length is 3 mod 4, add = padding
            inputurl += "=";
            break;
        default: // If the length is any other value mod 4, return an empty string
            return r;
        }

        boost::replace_all(inputurl, "_", "/"); // Replace all _ with /
        boost::replace_all(inputurl, "-", "+"); // Replace all - with +
    }
    ////////////////////
    // Create a memory BIO object with the base64-encoded data
    BIO *bio = BIO_new_mem_buf(inputx->data(), safe_cast_or(inputx->size(), 0));
    if (bio != nullptr)
    {
        // Create a BIO object for base64 decoding
        BIO *bio_base64 = BIO_new(BIO_f_base64());
        if (bio_base64 != nullptr)
        {
            // Link the BIO objects, so that the input to the decoding BIO object comes from the memory BIO object
            bio = BIO_push(bio_base64, bio);

            // Set the BIO_FLAGS_BASE64_NO_NL flag so that no newline characters are expected in the input
            BIO_set_flags(bio_base64, BIO_FLAGS_BASE64_NO_NL);

            // Read the base64-encoded data and decode it
            int decoded_size = BIO_read(bio, r->data, safe_cast_or(inputx->size(), 0));
            if (decoded_size >= 0)
            {
                // Update the length of the BinaryDataContainer object to reflect the actual decoded size
                r->length = decoded_size;
            }

            //BIO_free(bio_base64);
        }

        // Free the BIO objects
        BIO_free_all(bio);
    }

    // Return the decoded data as a shared pointer to the BinaryDataContainer object
    return r;
}

std::string Encoders::decodeFromBase64(std::string_view input, bool url)
{
    std::string inputurl;

    if (url) // Convert Base64URL to standard Base64
    {
        inputurl.assign(input); // Create a mutable copy of the input

        // Check the length of the input URL and add padding if necessary
        switch (inputurl.size() % 4)
        {
        case 0: // If evenly divisible by 4, do nothing
            break;
        case 2: // If 2 mod 4, pad with ==
            inputurl += "==";
            break;
        case 3: // If 3 mod 4, pad with =
            inputurl += "=";
            break;
        default: // Invalid Base64URL length
            return {};
        }

        // Replace URL-safe Base64 characters with standard Base64 characters
        for (char &c : inputurl)
        {
            if (c == '_')
            {
                c = '/';
            }
            else if (c == '-')
            {
                c = '+';
            }
        }

        input = inputurl; // Point the string_view to the converted input
    }

    // Check for empty input and OpenSSL's integer size limit
    if (input.empty() || input.size() > INT_MAX)
    {
        return {};
    }

    // Create a memory BIO object using the input data (no copy required)
    BIO *bio = BIO_new_mem_buf(input.data(), static_cast<int>(input.size()));
    if (bio == nullptr)
    {
        return {};
    }

    // Create a BIO object for Base64 decoding
    BIO *bio_base64 = BIO_new(BIO_f_base64());
    if (bio_base64 == nullptr)
    {
        BIO_free(bio);
        return {};
    }

    // Link the BIO objects so that the Base64 decoder reads from the memory BIO
    bio = BIO_push(bio_base64, bio);

    // Set the BIO_FLAGS_BASE64_NO_NL flag so that no newline characters are expected
    BIO_set_flags(bio_base64, BIO_FLAGS_BASE64_NO_NL);

    // Allocate enough space to store the maximum possible decoded data
    std::string result;
    result.resize((input.size() / 4) * 3);

    // Read and decode the Base64-encoded data
    int decoded_size = BIO_read(bio, result.data(), static_cast<int>(result.size()));

    // Free the entire BIO chain
    BIO_free_all(bio);

    // Return an empty string if decoding failed or produced no data
    if (decoded_size <= 0)
    {
        return {};
    }

    // Resize the result to the actual number of decoded bytes
    result.resize(static_cast<size_t>(decoded_size));

    // Return the decoded data as a string
    return result;
}

string Encoders::decodeFromBase32(std::string_view base32Value)
{
    std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string binarySecret;
    int currentByte = 0, bitsRemaining = 8;
    for (char c : base32Value)
    {
        if (c == '=')
        {
            break;
        }

        int val = chars.find(c);
        if (val == std::string::npos)
        {
            // Invalid character in base32 input
            return "";
        }

        // Assume that the base32 strings are well-formed
        int bits = 5;
        while (bits > 0)
        {
            int bit = (val >> (bits - 1)) & 1;
            currentByte = (currentByte << 1) | bit;
            bits--;
            bitsRemaining--;
            if (bitsRemaining == 0)
            {
                binarySecret.push_back(currentByte);
                bitsRemaining = 8;
                currentByte = 0;
            }
        }
    }

    if (bitsRemaining != 8 && bitsRemaining < 5)
    {
        currentByte = currentByte << (bitsRemaining + 3);
        binarySecret.push_back(currentByte);
    }

    return binarySecret;
}

string Encoders::encodeToBase64(std::string_view buf, bool url)
{
    return encodeToBase64(reinterpret_cast<const unsigned char *>(buf.data()), buf.size(), url);
}

string Encoders::encodeToBase64(const unsigned char *buf, size_t count, bool url)
{
    std::string result;

    // Create a BIO object for base64 encoding
    BIO *bio = BIO_new(BIO_f_base64());
    if (bio != nullptr)
    {
        // Create a BIO object for storing the output in memory
        BIO *bio_mem = BIO_new(BIO_s_mem());

        if (bio_mem)
        {
            // Link the BIO objects, so that the output of base64 encoding is written to the memory object
            BIO_push(bio, bio_mem);

            // Set the BIO_FLAGS_BASE64_NO_NL flag so that no newline characters are added to the output
            BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

            // Write the data to the base64 encoding BIO object
            int ret = BIO_write(bio, buf, safe_cast_or(count, 0));
            if (ret > 0)
            {
                // Flush any internal buffer used by the base64 encoding BIO object
                ret = BIO_flush(bio);
                if (ret == 1)
                {
                    // Get a pointer to the BUF_MEM object used by the memory BIO object
                    BUF_MEM *buffer_ptr;
                    ret = BIO_get_mem_ptr(bio_mem, &buffer_ptr);
                    if (ret == 1 && buffer_ptr != nullptr && buffer_ptr->length > 0)
                    {
                        // Create a string from the content of the memory buffer
                        result.assign(buffer_ptr->data, buffer_ptr->length);
                    }
                }
            }
            // Free the BIO objects
            //BIO_free(bio_mem);
        }
        BIO_free_all(bio);
    }

    if (url)
    {
        boost::replace_all(result, "/", "_");
        boost::replace_all(result, "+", "-");
        result.erase(std::remove_if(result.begin(), result.end(), [](char c) { return c == '='; }), result.end());
    }

    // Return the base64-encoded string
    return result;
}

string Encoders::toURL(std::string_view str, const Type &urlEncodingType)
{
    if (str.empty())
    {
        return "";
    }

    size_t x = 0;
    string out;
    out.resize(calcURLEncodingExpandedStringSize(str, urlEncodingType), ' ');

    for (char i : str)
    {
        if (getIfMustBeURLEncoded(i, urlEncodingType))
        {
            out[x++] = '%';
            out[x++] = toHexFrom4bitChar(i, 1);
            out[x++] = toHexFrom4bitChar(i, 2);
        }
        else
        {
            out[x++] = i;
        }
    }
    return out;
}

string Encoders::fromURL(std::string_view urlEncodedStr)
{
    std::string r;
    if (urlEncodedStr.empty())
    {
        return "";
    }

    for (size_t i = 0; i < urlEncodedStr.size(); i++)
    {
        if (urlEncodedStr[i] == '%' && i + 3 <= urlEncodedStr.size() && isxdigit(urlEncodedStr[i + 1]) && isxdigit(urlEncodedStr[i + 2]))
        {
            char v = hexToValue(urlEncodedStr[i + 1]) * 0x10 + hexToValue(urlEncodedStr[i + 2]);
            r += v;
            i += 2;
        }
        else
        {
            r += urlEncodedStr[i];
        }
    }
    return r;
}

string Encoders::htmlEscape(std::string_view input)
{
    std::string output(input);
    // Order matters: & must be replaced first to avoid double-escaping
    boost::replace_all(output, "&", std::string("&") + "amp;");
    boost::replace_all(output, "<", std::string("&") + "lt;");
    boost::replace_all(output, ">", std::string("&") + "gt;");
    boost::replace_all(output, "\"", std::string("&") + "quot;");
    boost::replace_all(output, "'", std::string("&") + "#x27;");
    return output;
}

string Encoders::toHex(const unsigned char *data, size_t len)
{
    string r;
    for (size_t x = 0; x < len; x++)
    {
        char buf[4];
        sprintf(buf, "%02" PRIX8, data[x]);
        r.append(buf);
    }
    return r;
}

void Encoders::replaceHexCodes(std::string &content)
{
    size_t pos = 0;

    // Manually search for the pattern "\\0x" followed by two hexadecimal characters
    while ((pos = content.find("\\0x", pos)) != std::string::npos)
    {
        // Verify that there are two hexadecimal characters following "\\0x"
        if (pos + 4 < content.size() && std::isxdigit(content[pos + 3]) && std::isxdigit(content[pos + 4]))
        {
            // Extract the two hexadecimal characters
            char hexcodes[3] = {content[pos + 3], content[pos + 4], 0};

            // Convert the two hexadecimal characters to an ASCII character
            unsigned char replSrc = hexPairToByte(hexcodes);

            // Replace the pattern "\\0xXX" with the corresponding ASCII character
            content.replace(pos, 5, std::string(1, replSrc));
        }
        else
        {
            // Advance if a valid pattern is not found
            pos += 3;
        }
    }
}

void Encoders::fromHex(std::string_view hexValue, unsigned char *data, size_t maxlen)
{
    if ((hexValue.size() / 2) < maxlen)
    {
        maxlen = (hexValue.size() / 2);
    }
    for (size_t i = 0; i < (maxlen * 2); i += 2)
    {
        data[i / 2] = hexToValue(hexValue.at(i)) * 0x10 + hexToValue(hexValue.at(i + 1));
    }
}

char Encoders::toHexFrom4bitChar(char nibble, const char &position)
{
    // Extract the high or low nibble from the byte, depending on the position.
    if (position == 1)
    {
        nibble = nibble / 0x10;
    }
    else if (position == 2)
    {
        nibble = nibble & 0xF;
    }

    // Convert the nibble to a hexadecimal character and return it.
    if (nibble >= 0x0 && nibble <= 0x9)
    {
        return '0' + nibble;
    }
    else if (nibble >= 0xA && nibble <= 0xF)
    {
        return 'A' + nibble - 0xA;
    }
    else
    {
        return '0';
    }
}

constexpr char Encoders::hexToValue(char v) noexcept
{
    if (v >= '0' && v <= '9')
    {
        return v - '0';
    }

    v |= 0x20; // Convert uppercase ASCII letters to lowercase

    return (v >= 'a' && v <= 'f') ? v - 'a' + 10 : 0;
}

unsigned char Encoders::hexPairToByte(const char *bytes)
{
    // Invalid HEX Code:
    if (!isxdigit(bytes[0]) || !isxdigit(bytes[1]))
    {
        return 0;
    }

    // Valid HEX Code:
    char hexStr[3] = {static_cast<char>(bytes[0]), static_cast<char>(bytes[1]), '\0'};
    return static_cast<unsigned char>(strtol(hexStr, nullptr, 16));
}

bool Encoders::getIfMustBeURLEncoded(char c, const Type &urlEncodingType)
{
    if (urlEncodingType == Type::QUOTEPRINT_ENCODING)
    {
        // All printable chars but "
        if (c == '\"')
        {
            return true;
        }
        if (c >= 32 && c <= 126)
        {
            return false;
        }
    }
    else
    {
        // be strict: Only very safe chars...
        if (isalnum(c))
        {
            return false;
        }
    }

    return true;
}

size_t Encoders::calcURLEncodingExpandedStringSize(std::string_view str, const Type &urlEncodingType)
{
    size_t x = 0;
    for (char i : str)
    {
        if (getIfMustBeURLEncoded(i, urlEncodingType))
        {
            x += 3;
        }
        else
        {
            x += 1;
        }
    }
    return x;
}
