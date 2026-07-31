#pragma once

#include "json/value.h"
#include <Mantids30/Memory/streamable_object.h>

#include <json/json.h>

#include <cstdint>
#include <functional>
#include <memory> // std::unique_ptr / std::shared_ptr were used without including this
#include <optional>
#include <string>
#include <vector>

namespace Mantids30::Scripts {

using ApiCallback_t = std::function<Json::Value(
    const std::string &baseApiUrl,
    const uint32_t &apiVersion,
    const std::string &methodType,
    const std::string &endpointName,
    const Json::Value &postParameters)>;


class MantidsLang : public Mantids30::Memory::Streams::StreamableObject
{
public:
    struct Token
    {
        enum class Type : uint8_t {
            DATA,
            SUBTAG
        } type = Type::DATA;
        std::unique_ptr<MantidsLang> subTag = nullptr;
        std::string tagName;
        std::vector<char> buffer;
    };

    MantidsLang(const std::shared_ptr<Memory::Streams::StreamableObject> &source,
                const std::shared_ptr<Json::Value> &jsonContext = nullptr,
                MantidsLang *parent = nullptr,
                ApiCallback_t apiCallback = nullptr);

    // You set the lang with the source and then stream this to an object...
    bool streamTo(Memory::Streams::StreamableObject *out) override;

    // By default, don't write here:
    std::optional<size_t> write(const void *buf, const size_t &count) override;

    ApiCallback_t apiCallback;

    bool isEmpty() const;

private:

    bool printJSON( const std::string &action , const Json::Value & value);
    bool iterateJSON(const std::string &foreachParam, const Json::Value &currentJsonContext, const Token &token, size_t depth);
    bool checkConditionalOnJSON(const std::string &conditional, const Json::Value &currentJsonContext, const Token &token, size_t depth);

    // depth is only used for the (testing) indented dump.
    void processTokens(const Json::Value & currentJsonContext, size_t depth = 0);
    void parseTagsFromBuffer();

    // Parse-only recursion for EOF: flushes pending buffers of the open subtag chain.
    void finalizeParsing();

    void addSubTagToken(const std::string &tagName, bool activate);
    void createSubTag(const std::string &tagName);
    void closeSubTag(const std::string &dataAfterClosing);

    // Guarantees that tokens.back() is a writable DATA token.
    Token &currentDataToken();

    //std::string tag;
    std::string expectedClosingTag;

    // INSIDE TAG STATUS:
    std::vector<Token> tokens;

    std::shared_ptr<Memory::Streams::StreamableObject> source;
    std::shared_ptr<Json::Value> jsonContext;
    Memory::Streams::StreamableObject *output = nullptr;

    // Parent pointer for nested context tracking
    MantidsLang *parent = nullptr;

    // Current active subtag (when inside a subtag, writes go here)
    MantidsLang *activeSubTag = nullptr;

    // EOF state
    bool eofReached = false;

    // This subtag already found its {{/NAME}}; it must not accept more data.
    bool closed = false;
};

} // namespace Mantids30::Scripts