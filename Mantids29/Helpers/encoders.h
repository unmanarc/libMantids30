#ifndef HLP_ENCODERS_H
#define HLP_ENCODERS_H

#include <memory>
#include <string>
#include "mem.h"

namespace Mantids29 { namespace Helpers {

/**
 * @brief A class that provides functions for encoding and decoding data in various formats.
 */
class Encoders
{
public:
    /**
     * @brief An enumeration of the URL encoding types that can be used.
     */
    enum URL_ENCODING_TYPE
    {
        STRICT_ENCODING,
        QUOTEPRINT_ENCODING
    };

    /**
     * @brief Constructs a new Encoders object.
     */
    Encoders();

    /**
     * @brief Performs an obfuscated base64 encoding on binary data.
     *
     * @param sB64Buf The input binary data to encode.
     * @param seed The seed value to use for the encoding (default: 0xAA12017BEA385A7B).
     *
     * @return The obfuscated base64-encoded string.
     */
    static std::string decodeFromBase64Obf(std::string const& sB64Buf, const uint64_t& seed = 0xAA12017BEA385A7B);

    /**
     * @brief Performs an obfuscated base64 encoding on binary data.
     *
     * @param buf A pointer to the input binary data to encode.
     * @param count The length of the input binary data in bytes.
     * @param seed The seed value to use for the encoding (default: 0xAA12017BEA385A7B).
     *
     * @return The obfuscated base64-encoded string.
     */
    static std::string encodeToBase64Obf(unsigned char const* buf, uint64_t count, const uint64_t& seed = 0xAA12017BEA385A7B);

    /**
     * @brief Performs an obfuscated base64 encoding on a string.
     *
     * @param buf The input string to encode.
     * @param seed The seed value to use for the encoding (default: 0xAA12017BEA385A7B).
     *
     * @return The obfuscated base64-encoded string.
     */
    static std::string encodeToBase64Obf(const std::string& buf, const uint64_t& seed = 0xAA12017BEA385A7B);

    /**
     * @brief Decodes a base64-encoded string to binary data.
     *
     * @param sB64Buf The base64-encoded string to decode.
     *
     * @return A shared pointer to a BinaryDataContainer object containing the decoded binary data.
     */
    static std::shared_ptr<Mem::BinaryDataContainer> decodeFromBase64ToBin(std::string const& input, bool url = false);

    /**
     * @brief Decodes a base64-encoded string.
     *
     * @param sB64Buf The base64-encoded string to decode.
     *
     * @return The decoded string.
     */
    static std::string decodeFromBase64(std::string const& input, bool url = false);


    /**
     * @brief Decodes a base32-encoded string.
     *
     * @param base32Value The base32-encoded string to decode.
     *
     * @return The decoded string.
     */
    static std::string decodeFromBase32(const std::string &base32Value);




    /**
     * @brief Performs a base64 encoding on a string.
     *
     * @param buf The input string to encode.
     *
     * @return The base64-encoded string.
     */
    static std::string encodeToBase64(const std::string& buf, bool url = false);

    /**
     * @brief Performs a base64 encoding on binary data.
     *
     * @param buf A pointer to the input binary data to encode.
     * @param count The length of the input binary data in bytes.
     *
     * @return The base64-encoded string.
     */
    static std::string encodeToBase64(unsigned char const* buf, uint64_t count, bool url = false);

    /**
     * @brief Encodes a string for use in a URL.
     *
     * @param str The input string to encode.
     * @param urlEncodingType The URL encoding type
     *
     * @return The encoded URL string.
     */
    static std::string toURL(const std::string& str, const URL_ENCODING_TYPE& urlEncodingType = STRICT_ENCODING);

    /**
     * @brief Decodes a string that has been encoded for use in a URL.
     *
     * @param urlEncodedStr The URL-encoded string to decode.
     *
     * @return The decoded string.
     */
    static std::string fromURL(const std::string& urlEncodedStr);

    /**
     * @brief Performs a hexadecimal encoding on binary data.
     *
     * @param data A pointer to the input binary data to encode.
     * @param len The length of the input binary data in bytes.
     *
     * @return The hexadecimal-encoded string.
     */
    static std::string toHex(const unsigned char* data, size_t len);

    /**
     * @brief Decodes a string that has been hexadecimal-encoded to binary data.
     *
     * @param hexValue The hexadecimal-encoded string to decode.
     * @param data A pointer to the buffer to store the decoded binary data in.
     * @param maxlen The maximum length of the buffer.
     */
    static void fromHex(const std::string& hexValue, unsigned char* data, size_t maxlen);

    /**
     * @brief Converts a 4-bit nibble to an ASCII hexadecimal character.
     *
     * @param nibble The 4-bit nibble to convert.
     * @param position The position of the nibble within the byte (1 or 2).
     *
     * @return The ASCII hexadecimal character corresponding to the nibble.
     */
    static char toHexFrom4bitChar(char value, char part);

    /**
     * @brief Determines whether a character is a hexadecimal character.
     *
     * @param v The character to check.
     *
     * @return True if the character is a hexadecimal character, false otherwise.
     */
    static bool isHex(char v);

    /**
     * @brief Converts a hexadecimal character to its corresponding byte value.
     *
     * @param v The hexadecimal character to convert.
     *
     * @return The byte value corresponding to the hexadecimal character.
     */
    static char hexToValue(char v);

private:
    /**
     * @brief Determines whether a character must be URL-encoded.
     *
     * @param c The character to check.
     * @param urlEncodingType The URL encoding type to use.
     *
     * @return True if the character must be URL-encoded, false otherwise.
     */
    static bool getIfMustBeURLEncoded(char c, const URL_ENCODING_TYPE& urlEncodingType);

    /**
     * @brief Calculates the size of an expanded URL-encoded string.
     *
     * @param str The input string to encode.
     * @param urlEncodingType The URL encoding type to use.
     *
     * @return The size of the expanded URL-encoded string.
     */
    static size_t calcURLEncodingExpandedStringSize(const std::string& str, const URL_ENCODING_TYPE& urlEncodingType);

    /**
     * @brief The set of characters used in base64 encoding.
     */
    static const std::string b64Chars;
};

}}

#endif // HLP_ENCODERS_H
