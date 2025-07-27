#pragma once

#include "subparser.h"
#include <memory>

namespace Mantids30 { namespace Memory { namespace Streams {


/**
 * @brief The ProtocolParser_Base class
 *
 * Via Object:
 *                                             *------------------*
 * Current Parser <---- Binary <--- Parser <---|    Streamable    |
 *                      Object                 *------------------*
 *
 * Via SO: (you can use writeStream and streamTo)
 *
 * Current Parser <---- Binary <--- Parser (Streamable)
 *                      Object
 *
 *
 */
class Parser : public Memory::Streams::StreamableObject
{
public:
    Parser(std::shared_ptr<Memory::Streams::StreamableObject> value, bool clientMode);
    virtual ~Parser() override = default;
    enum ErrorMSG {
        PARSING_SUCCEED = 0,
        PARSING_ERR_INIT = -1,
        PARSING_ERR_PARSING = -2,
    };

    /**
     * @brief parseObject Parse streamable object (init/parse/end)
     * @param err: (0:succeed, -1:failed to initialize, -2:failed to read, -3:failed to parse/write)
     */
    void parseObject(ErrorMSG *err);

    //////////////////////////////////////////
    virtual std::optional<size_t> write(const void *buf, const size_t &count) override;

    //////////////////////////////////////////
    /**
     * @brief setMaxTTL Set Max TTL for parsing data
     * @param value max times parseData should be called
     */
    void setMaxTTL(const size_t &value);

    void setStreamable(std::shared_ptr<StreamableObject> value);

protected:
    // Avoid to copy streaming things...
    Parser& operator=(const Parser&)
    {
        return *this; // NO-OP Copy...
    }
    Parser(Parser&) = delete;

    //////////////////////////////////////////
    // Virtual functions to initialize the protocol.
    virtual bool initProtocol() = 0;
    virtual void endProtocol() = 0;
    virtual bool changeToNextParser() = 0;

    std::shared_ptr<Memory::Streams::StreamableObject> m_streamableObject;

    void initSubParser(SubParser * subparser);

    SubParser * m_currentParser = nullptr;
    size_t m_maxTTL = 4096;
    bool m_initialized = false;
    bool m_clientMode;

private:
    /**
     * @brief parseData parse data into current parser. This calls recursively
     *                  until all data is filled.
     * @param buf data to be parsed
     * @param count data size to be parsed
     * @param ttl Time To Live Counter.
     * @return -1 if error, and n>0 : n length of data processed by parser, which should be equal to count.
     */
    std::optional<size_t> parseData(const void * buf, size_t count, size_t *ttl, bool * finished);
};



}}}



