#pragma once

#include "b_chunks.h"
#include "b_ref.h"
#include "streamable_object.h"
#include <optional>

//#define DEBUG_PARSER 1

#ifdef DEBUG_PARSER
#include <openssl/bio.h>
#define BIO_dump_fp(a, b, c) \
    fprintf(a, "BIO_dump_fp->%lu bytes\n", c); \
    fflush(a);
#endif

namespace Mantids30::Memory::Streams {

class SubParser
{
public:
    enum class ParseResult : uint8_t
    {
        ERROR,
        GET_MORE_DATA,
        GOTO_NEXT_SUBPARSER
        // TODO: ParseResult::STAT_NEXT_MAINPARSER
    };

    enum class ParseStrategy : uint8_t
    {
        DELIMITER,        // wait for delimiter
        SIZE,             // wait for size
        VALIDATOR,        // wait for validation (TODO)
        CONNECTION_END,   // wait for connection end
        DIRECT,           // don't wait, just parse.
        DIRECT_DELIMITER, // parse direct until multi-delimiter (// TODO:)
        MULTIDELIMITER    //,       // wait for any of those delimiters
                                     //        ParseStrategy::FREEPARSER             // TODO
    };

    SubParser() = default;
    SubParser(SubParser &) = delete;
    virtual ~SubParser() = default;

    void initElemParser(StreamableObject *upStreamObj, bool clientMode);

    ///////////////////
    /**
     * @brief getParseStatus
     * @return
     */
    ParseResult getParseStatus() const;
    /**
     * @brief Set Parse Status
     * @param value
     */
    void setParseResult(const ParseResult &value);
    /**
     * Write Stream Data Into Object.
     * @param buf binary data to be written on the object.
     * @param count size in bytes to be written on the object. If 0, it will indicate that stream ended.
     * @return -1 if failed, or bytes written >0. 0 on ending stream.
     */
    // TODO: size_t -1? why not ssize_t?
    std::optional<size_t> writeIntoSubParser(const void *buf, size_t count);
    /**
     * @brief stream Virtual function to stream this sub parser into upstream object
     * @return true if succeed
     */
    virtual bool streamToUpstream()
    {
        // if not implemented...
        return false;
    }
    /**
     * @brief Returns the size of the unparsed data remaining in the buffer.
     * @return Size of the unparsed data in bytes.
     */
    size_t getUnparsedDataSize();
    /**
     * @brief Returns the delimiter string that was found during parsing with multiple delimiters.
     * @return The delimiter string that was matched in the current parsing context.
     */
    std::string getFoundDelimiter() const;
    /**
     * @brief isStreamEnded Get if the last piece of data of the stream was parsed.
     * @return
     */
    bool isStreamEnded() const;

    const std::string &getSubParserName() const;

protected:
    // Avoid to copy streaming things...
    SubParser &operator=(const SubParser &)
    {
        return *this; // NO-OP Copy...
    }

    /**
     * @brief Parse is called when the parser in writeStream reached the completion of ParseMode
     * @return Parse Status (ERROR: Parse Error, OK_CONTINUE: Ok, continue parsing, OK_END: )
     */
    virtual ParseResult parse() = 0;
    /**
     * @brief ParseValidator On Parsing mode Validator, Parse Validator should return the size of the content that matches with the validation (eg. xml, json)
     * @param bc container with the data to be analyzed.
     * @return bytes matching the policy.
     */
    virtual size_t ParseValidator(Mantids30::Memory::Containers::B_Base &);
    /**
     * @brief Set Parse Mode
     * @param value parse mode (delimiter, size, or validator)
     */
    void setParseStrategy(const ParseStrategy &value);
    /**
     * @brief setParseDelimiter Set Parse Delimiter
     * @param value Parse Delimiter
     */
    void setParseDelimiter(const std::string &value);
    /**
     * @brief setParseMultiDelimiter Set Parse multi delimiter parameter (parse many delimiters)
     * @param value multiple delimiters
     */
    void setParseMultiDelimiter(const std::list<std::string> &value);
    /**
     * @brief Get Parsed Data Pointer
     * @return parsed data pointer.
     */
    Mantids30::Memory::Containers::B_Base *getParsedBuffer();
    /**
     * @brief Set Parse Data Target Size in bytes
     * @param value Target Size in bytes
     */
    void setParseDataTargetSize(const size_t &value);

    /**
     * @brief clear Clear parsed and unparsed buffers... (ready to use it again)
     */
    void clear();

    ////////////////////////////
    bool m_clientMode = true;
    bool m_streamEnded = false;
    std::string m_subParserName;
    Memory::Streams::StreamableObject *m_upStream = nullptr;

private:
    std::optional<size_t> parseByMultiDelimiter(const void *buf, size_t count);
    std::optional<size_t> parseByDelimiter(const void *buf, size_t count);
    std::optional<size_t> parseBySize(const void *buf, size_t count);
    std::optional<size_t> parseByValidator(const void *, size_t);
    std::optional<size_t> parseByConnectionEnd(const void *buf, size_t count);
    std::optional<size_t> parseDirect(const void *buf, size_t count);
    std::optional<size_t> parseDirectDelimiter(const void *buf, size_t count);

    size_t getLastBytesInCommon(const std::string &boundary);
    Mantids30::Memory::Containers::B_Ref referenceLastBytes(const size_t &bytes);

    std::optional<size_t> appendToUnparsedBuffer(const void *buf, size_t count);

    Mantids30::Memory::Containers::B_Ref m_parsedBuffer;
    Mantids30::Memory::Containers::B_Chunks m_unparsedBuffer;

    std::string m_parseDelimiter = "\r\n";
    std::string m_delimiterFound;

    std::list<std::string> m_parseMultiDelimiter;

    ParseStrategy m_parseMode = ParseStrategy::DELIMITER;
    ParseResult m_parseStatus = ParseResult::GET_MORE_DATA;
};

} // namespace Mantids30::Memory::Streams
