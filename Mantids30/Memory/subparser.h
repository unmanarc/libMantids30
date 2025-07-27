#pragma once

#include "b_chunks.h"
#include "b_ref.h"
#include "streamableobject.h"
#include <optional>

//#define DEBUG_PARSER 1

#ifdef DEBUG_PARSER
#include <openssl/bio.h>
#define BIO_dump_fp(a,b,c) fprintf(a, "BIO_dump_fp->%lu bytes\n", c); fflush(a);
#endif


namespace Mantids30 { namespace Memory { namespace Streams {


class SubParser
{
public:
    enum ParseStatus {
        PARSE_ERROR,
        PARSE_GET_MORE_DATA,
        PARSE_GOTO_NEXT_SUBPARSER
        // TODO: PARSE_STAT_NEXT_MAINPARSER
    };

    enum ParseMode {
        PARSE_MODE_DELIMITER,               // wait for delimiter
        PARSE_MODE_SIZE,                    // wait for size
        PARSE_MODE_VALIDATOR,               // wait for validation (TODO)
        PARSE_MODE_CONNECTION_END,          // wait for connection end
        PARSE_MODE_DIRECT,                  // don't wait, just parse.
        PARSE_MODE_DIRECT_DELIMITER,   // parse direct until multi-delimiter (// TODO:)
        PARSE_MODE_MULTIDELIMITER //,          // wait for any of those delimiters
//        PARSE_MODE_FREEPARSER               // TODO
    };

    SubParser() = default;
    virtual ~SubParser() = default;

    void initElemParser(StreamableObject *upStreamObj, bool clientMode);

    ///////////////////
    /**
     * @brief getParseStatus
     * @return
     */
    ParseStatus getParseStatus() const;
    /**
     * @brief Set Parse Status
     * @param value
     */
    void setParseStatus(const ParseStatus &value);
    /**
     * Write Stream Data Into Object.
     * @param buf binary data to be written on the object.
     * @param count size in bytes to be written on the object. If 0, it will indicate that stream ended.
     * @return -1 if failed, or bytes written >0. 0 on ending stream.
     */
    // TODO: size_t -1? why not ssize_t?
    std::optional<size_t> writeIntoParser(const void * buf, size_t count);
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
    SubParser& operator=(const SubParser&)
    {
        return *this; // NO-OP Copy...
    }
    SubParser(SubParser&) = delete;

    /**
     * @brief Parse is called when the parser in writeStream reached the completion of ParseMode
     * @return Parse Status (ERROR: Parse Error, OK_CONTINUE: Ok, continue parsing, OK_END: )
     */
    virtual ParseStatus parse() = 0;
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
    void setParseMode(const ParseMode &value);
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
    Mantids30::Memory::Containers::B_Base * getParsedBuffer();
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
    Memory::Streams::StreamableObject * m_upStream = nullptr;

private:
    std::optional<size_t> parseByMultiDelimiter(const void * buf, size_t count);
    std::optional<size_t> parseByDelimiter(const void * buf, size_t count);
    std::optional<size_t> parseBySize(const void * buf, size_t count);
    std::optional<size_t> parseByValidator(const void *, size_t);
    std::optional<size_t> parseByConnectionEnd(const void * buf, size_t count);
    std::optional<size_t> parseDirect(const void * buf, size_t count);
    std::optional<size_t> parseDirectDelimiter(const void * buf, size_t count);

    size_t getLastBytesInCommon(const std::string &boundary);
    Mantids30::Memory::Containers::B_Ref referenceLastBytes(const size_t &bytes);
    Mantids30::Memory::Containers::B_Ref m_parsedBuffer;
    Mantids30::Memory::Containers::B_Chunks m_unparsedBuffer;

    std::string m_parseDelimiter  = "\r\n";
    std::string m_delimiterFound;

    std::list<std::string> m_parseMultiDelimiter;

    ParseMode m_parseMode = PARSE_MODE_DELIMITER;
    ParseStatus m_parseStatus = PARSE_GET_MORE_DATA;
};

}}}

