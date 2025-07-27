#pragma once

#include "mem.h"
#include <memory>
#include <string>

namespace Mantids30 {
namespace Helpers {

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
     * @brief Performs an obfuscated base64 encoding on binary data.
     *
     * @param sB64Buf The input binary data to encode.
     * @param seed The seed value to use for the encoding (default: 0xAA12017BEA385A7B).
     *
     * @return The obfuscated base64-encoded string.
     */
    static std::string decodeFromBase64Obf(std::string const &sB64Buf, const uint64_t &seed = 0xAA12017BEA385A7B);

    /**
     * @brief Performs an obfuscated base64 encoding on binary data.
     *
     * @param buf A pointer to the input binary data to encode.
     * @param count The length of the input binary data in bytes.
     * @param seed The seed value to use for the encoding (default: 0xAA12017BEA385A7B).
     *
     * @return The obfuscated base64-encoded string.
     */
    static std::string encodeToBase64Obf(unsigned char const *buf, size_t count, const uint64_t &seed = 0xAA12017BEA385A7B);

    /**
     * @brief Performs an obfuscated base64 encoding on a string.
     *
     * @param buf The input string to encode.
     * @param seed The seed value to use for the encoding (default: 0xAA12017BEA385A7B).
     *
     * @return The obfuscated base64-encoded string.
     */
    static std::string encodeToBase64Obf(const std::string &buf, const uint64_t &seed = 0xAA12017BEA385A7B);

    /**
     * @brief Decodes a base64-encoded string to binary data.
     *
     * This function decodes a given base64-encoded string into its original binary form. If the input is URL-safe base64 encoded,
     * set the `url` parameter to true, otherwise leave it as false (default).
     *
     * @param input The base64-encoded string to decode.
     * @param url A boolean indicating whether the input is URL-safe base64 encoded (default: false).
     *
     * @return A shared pointer to a BinaryDataContainer object containing the decoded binary data. If decoding fails,
     *         an empty shared pointer is returned.
     */
    static std::shared_ptr<Mem::BinaryDataContainer> decodeFromBase64ToBin(std::string const &input, bool url = false);

    /**
     * @brief Decodes a base64-encoded string.
     *
     * This function decodes a given base64-encoded string into its original string form. If the input is URL-safe base64 encoded,
     * set the `url` parameter to true, otherwise leave it as false (default).
     *
     * @param input The base64-encoded string to decode.
     * @param url A boolean indicating whether the input is URL-safe base64 encoded (default: false).
     *
     * @return The decoded string. If decoding fails, an empty string is returned.
     */
    static std::string decodeFromBase64(std::string const &input, bool url = false);

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
    static std::string encodeToBase64(const std::string &buf, bool url = false);
    /**
     * @brief Performs a base64 encoding on binary data.
     *
     * @param buf A pointer to the input binary data to encode.
     * @param count The length of the input binary data in bytes.
     *
     * @return The base64-encoded string.
     */
    static std::string encodeToBase64(unsigned char const *buf, size_t count, bool url = false);

    /**
     * @brief Encodes a string for use in a URL.
     *
     * @param str The input string to encode.
     * @param urlEncodingType The URL encoding type
     *
     * @return The encoded URL string.
     */
    static std::string toURL(const std::string &str, const URL_ENCODING_TYPE &urlEncodingType = STRICT_ENCODING);

    /**
     * @brief Decodes a string that has been encoded for use in a URL.
     *
     * @param urlEncodedStr The URL-encoded string to decode.
     *
     * @return The decoded string.
     */
    static std::string fromURL(const std::string &urlEncodedStr);

    /**
     * @brief Performs a hexadecimal encoding on binary data.
     *
     * @param data A pointer to the input binary data to encode.
     * @param len The length of the input binary data in bytes.
     *
     * @return The hexadecimal-encoded string.
     */
    static std::string toHex(const unsigned char *data, size_t len);

     /**
     * @brief Replaces hexadecimal codes within the content string with their corresponding ASCII characters.
     *
     * This function scans the input string `content` for patterns matching the format `\0xXX`, where `XX`
     * represents two hexadecimal characters. When such a pattern is found, it is replaced with the
     * corresponding ASCII character.
     *
     * @param content Reference to a `std::string` that will be modified by replacing hexadecimal codes with ASCII characters.
     *
     * The function performs the following steps:
     * - It iteratively searches for occurrences of the pattern `\0x` followed by two hexadecimal characters.
     * - For each match, it verifies that the two characters following `\0x` are valid hexadecimal digits.
     * - If valid, it converts these two hexadecimal characters into an `unsigned char` using the `hexPairToByte` function.
     * - The matched pattern is then replaced by the resulting ASCII character within the string.
     * - If no valid hexadecimal pair is found, it advances to continue the search.
     *
     * Example usage:
     * @code
     * std::string text = "Example \\0x41 text";
     * Encoders::replaceHexCodes(text);
     * // text now contains "Example A text"
     * @endcode
     */
    static void replaceHexCodes(std::string &content);

    /**
     * @brief Decodes a string that has been hexadecimal-encoded to binary data.
     *
     * @param hexValue The hexadecimal-encoded string to decode.
     * @param data A pointer to the buffer to store the decoded binary data in.
     * @param maxlen The maximum length of the buffer.
     */
    static void fromHex(const std::string &hexValue, unsigned char *data, size_t maxlen);

    /**
     * @brief Converts a 4-bit nibble to an ASCII hexadecimal character.
     *
     * This function takes a 4-bit value and converts it to the corresponding
     * ASCII hexadecimal character. The function assumes that the input value is
     * a valid 4-bit nibble (i.e., in the range 0-15).
     *
     * @param value The 4-bit value to convert.
     * @param part Specifies which half of the byte is being processed:
     *             - `part` should be 1 if converting the higher 4 bits of a byte,
     *             - `part` should be 2 if converting the lower 4 bits of a byte.
     *
     * @return The ASCII hexadecimal character corresponding to the nibble.
     */
    static char toHexFrom4bitChar(char value, const char &part);

    /**
     * @brief Converts a hexadecimal character to its corresponding byte value.
     *
     * @param v The hexadecimal character to convert.
     *
     * @return The byte value corresponding to the hexadecimal character.
     */
    static char hexToValue(const char & v);

    /**
     * @brief Converts a pair of hexadecimal characters to an unsigned byte.
     *
     * This function takes a pointer to a character array where the first two characters represent
     * a hexadecimal number. It converts this two-character hexadecimal representation to a single
     * `unsigned char` (byte) value.
     *
     * @param bytes Pointer to a `char` array containing at least two hexadecimal characters.
     *              Assumes `bytes[0]` and `bytes[1]` are valid hexadecimal digits (0-9, A-F, a-f).
     * @return An `unsigned char` representing the byte value of the hexadecimal pair.
     *
     * @note This function does not perform error checking on the validity of the input characters.
     *       It assumes the caller has verified that `bytes` contains valid hexadecimal digits.
     *
     * Example usage:
     * @code
     * unsigned char byte = hexPairToByte("1A"); // Returns the byte 0x1A
     * @endcode
     */
    static unsigned char hexPairToByte(const char *bytes);

private:
    /**
     * @brief Determines whether a character must be URL-encoded.
     *
     * @param c The character to check.
     * @param urlEncodingType The URL encoding type to use.
     *
     * @return True if the character must be URL-encoded, false otherwise.
     */
    static bool getIfMustBeURLEncoded(char c, const URL_ENCODING_TYPE &urlEncodingType);

    /**
     * @brief Calculates the size of an expanded URL-encoded string.
     *
     * @param str The input string to encode.
     * @param urlEncodingType The URL encoding type to use.
     *
     * @return The size of the expanded URL-encoded string.
     */
    static size_t calcURLEncodingExpandedStringSize(const std::string &str, const URL_ENCODING_TYPE &urlEncodingType);

    /**
     * @brief The set of characters used in base64 encoding.
     */
    static const std::string m_b64Chars;
};

} // namespace Helpers
} // namespace Mantids30
