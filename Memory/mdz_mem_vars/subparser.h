#ifndef INTERNALPARSER_H
#define INTERNALPARSER_H

#include "b_chunks.h"
#include "b_ref.h"
#include "streamableobject.h"

//#define DEBUG 1

#ifdef DEBUG
#include <openssl/bio.h>
#define BIO_dump_fp(a,b,c) fprintf(a, "BIO_dump_fp->%lu bytes\n", c); fflush(a);
#endif


namespace Mantids { namespace Memory { namespace Streams {


class SubParser
{
public:
    enum ParseStatus {
        PARSE_STAT_ERROR,
        PARSE_STAT_GET_MORE_DATA,
        PARSE_STAT_GOTO_NEXT_SUBPARSER
        // TODO: PARSE_STAT_NEXT_MAINPARSER
    };

    enum ParseMode {
        PARSE_MODE_DELIMITER,               // wait for delimiter
        PARSE_MODE_SIZE,                    // wait for size
        PARSE_MODE_VALIDATOR,               // wait for validation (TODO)
        PARSE_MODE_CONNECTION_END,          // wait for connection end
        PARSE_MODE_DIRECT,                  // don't wait, just parse.
        PARSE_MODE_DIRECT_DELIMITER,   // parse direct until multi-delimiter (// TODO:)
        PARSE_MODE_MULTIDELIMITER,          // wait for any of those delimiters
        PARSE_MODE_FREEPARSER               // TODO
    };

    SubParser();
    virtual ~SubParser();
    void initElemParser(Memory::Streams::StreamableObject *upstreamObj, bool clientMode);

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
    std::pair<bool, uint64_t> writeIntoParser(const void * buf, size_t count);
    /**
     * @brief stream Virtual function to stream this sub parser into upstream object
     * @param wrStat updates bytes written and error status.
     * @return true if succeed
     */
    virtual bool stream(StreamableObject::Status & wrStat) = 0;
    /**
     * @brief getLeftToParse Get Left data to parse.
     * @return left data to parse.
     */
    uint64_t getLeftToparse() const;
    /**
     * @brief getDelimiterFound Get delimiter found on multi-delimiter.
     * @return delimiter string found.
     */
    std::string getDelimiterFound() const;
    /**
     * @brief isStreamEnded Get if the last piece of data of the stream was parsed.
     * @return
     */
    bool isStreamEnded() const;

    const std::string &getSubParserName() const;

protected:
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
    virtual size_t ParseValidator(Mantids::Memory::Containers::B_Base & bc);
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
    Mantids::Memory::Containers::B_Base * getParsedBuffer();
    /**
     * @brief Set Parse Data Target Size in bytes
     * @param value Target Size in bytes
     */
    void setParseDataTargetSize(const uint64_t &value);


    /**
     * @brief clear Clear parsed and unparsed buffers... (ready to use it again)
     */
    void clear();

    ////////////////////////////
    bool clientMode;
    Memory::Streams::StreamableObject * upStream;
    bool streamEnded;
    std::string subParserName;

private:
    std::pair<bool,uint64_t> parseByMultiDelimiter(const void * buf, size_t count);
    std::pair<bool,uint64_t> parseByDelimiter(const void * buf, size_t count);
    std::pair<bool,uint64_t> parseBySize(const void * buf, size_t count);
    std::pair<bool,uint64_t> parseByValidator(const void * buf, size_t count);
    std::pair<bool,uint64_t> parseByConnectionEnd(const void * buf, size_t count);
    std::pair<bool,uint64_t> parseDirect(const void * buf, size_t count);
    std::pair<bool,uint64_t> parseDirectDelimiter(const void * buf, size_t count);

    uint64_t getLastBytesInCommon(const std::string &boundary);
    Mantids::Memory::Containers::B_Ref referenceLastBytes(const size_t &bytes);

    Mantids::Memory::Containers::B_Ref parsedBuffer;
    Mantids::Memory::Containers::B_Chunks unparsedBuffer;
    std::string parseDelimiter;

    std::string delimiterFound;
    std::list<std::string> parseMultiDelimiter;

    ParseMode parseMode;
    ParseStatus parseStatus;
};

}}}

#endif // INTERNALPARSER_H
