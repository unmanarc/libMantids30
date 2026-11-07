#include "Mantids30/Memory/streamable_file.h"
#include "mantidslang.h"

#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/DataFormat_JSON/json.h>
#include <Mantids30/Memory/streamable_null.h>
#include <Mantids30/Scripts_JSONExprEval/jsoneval.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <fcntl.h>
#include <memory>

using namespace Mantids30;
using namespace Mantids30::Scripts;

bool MantidsLang::checkConditionalOnJSON(const std::string &conditional, const boost::json::value &currentJsonContext, const Token &token, size_t depth)
{
    size_t bracketStart = conditional.find('[');
    size_t bracketEnd = conditional.rfind(']');

    if (bracketStart == std::string::npos || bracketEnd == std::string::npos || bracketEnd <= bracketStart)
    {
        return true; // Invalid syntax, skip
    }

    std::string evaluationExpression = conditional.substr(bracketStart + 1, bracketEnd - bracketStart - 1);

    Expressions::JSONEval eval(evaluationExpression);
    if (eval.evaluate(currentJsonContext))
    {
        token.subTag->processTokens(currentJsonContext, depth + 1);
        if (!writeStatus.succeed)
        {
            return false;
        }
    }
    return true;
}

bool MantidsLang::iterateJSON(const std::string &foreachParam, const boost::json::value &currentJsonContext, const Token &token, size_t depth)
{
    // Parse "[json_path as varname]"
    size_t bracketStart = foreachParam.find('[');
    size_t bracketEnd = foreachParam.rfind(']');

    if (bracketStart == std::string::npos || bracketEnd == std::string::npos || bracketEnd <= bracketStart)
    {
        return true; // Invalid syntax, skip
    }

    std::string inner = foreachParam.substr(bracketStart + 1, bracketEnd - bracketStart - 1);

    // Parse "json_path as varname"
    size_t asPos = inner.find(" as ");
    if (asPos == std::string::npos)
    {
        return true; // Invalid syntax, skip
    }

    std::string jsonPathStr = inner.substr(0, asPos);
    std::string varName = inner.substr(asPos + 4);

    // Trim whitespace
    boost::trim(jsonPathStr);
    boost::trim(varName);

    if (!boost::starts_with(jsonPathStr,"$"))
    {
        // jsonPathStr should be in format: $json.path
        return true;
    }
    jsonPathStr = jsonPathStr.substr(1);


    // Resolve JSON path
    Helpers::JSON::Path path(jsonPathStr);
    const boost::json::value *collectionPtr = path.resolve(currentJsonContext);

    if (collectionPtr == nullptr || collectionPtr->is_null())
    {
        // Path not found, skip iteration
        return true;
    }

    const boost::json::value &collection = *collectionPtr;

    // Create a copy of the JSON context for each iteration
    boost::json::value newJsonContext;

    if (collection.is_array())
    {
        // Iterate over array elements
        for (size_t idx = 0; idx < collection.as_array().size(); ++idx)
        {
            newJsonContext = currentJsonContext; // Reset context for each iteration
            if (!newJsonContext.is_object())
            {
                newJsonContext = boost::json::object{};
            }
            newJsonContext.as_object()[varName] = collection.as_array()[idx];

            token.subTag->processTokens(newJsonContext, depth + 1);
            if (!writeStatus.succeed)
            {
                return false;
            }
        }
    }
    else if (collection.is_object())
    {
        // Iterate over object members (key-value pairs)
        for (const auto &kv : collection.as_object())
        {
            newJsonContext = currentJsonContext; // Reset context for each iteration
            boost::json::value iteratorObj = boost::json::object{};
            iteratorObj.as_object()["key"] = kv.key();
            iteratorObj.as_object()["value"] = kv.value();
            if (!newJsonContext.is_object())
            {
                newJsonContext = boost::json::object{};
            }
            newJsonContext.as_object()[varName] = iteratorObj;

            token.subTag->processTokens(newJsonContext, depth + 1);
            if (!writeStatus.succeed)
            {
                return false;
            }
        }
    }

    return true;
}

bool MantidsLang::printJSON(const std::string &action, const boost::json::value &value)
{
    if (action == "print_json")
    {
        // Output raw JSON representation
        if (!output->writeString(Helpers::JSON::toString(value)))
        {
            writeStatus.succeed = false;
            return false;
        }
    }
    else if (action == "print_htmlescaped" && !value.is_null())
    {
        // If leaf node (string/int/float/bool) -> HTML escape the value
        // Otherwise -> HTML escape the entire JSON representation
        if (value.is_string())
        {
            if (!output->writeString(Helpers::Encoders::htmlEscape(Helpers::JSON::ASSTRING_D(value, ""))))
            {
                writeStatus.succeed = false;
                return false;
            }
        }
        else if (value.is_int64() || value.is_int64() || value.is_uint64() || value.is_uint64())
        {
            if (!output->writeString(Helpers::JSON::ASSTRING_D(value, "")))
            {
                writeStatus.succeed = false;
                return false;
            }
        }
        else if (value.is_double())
        {
            if (!output->writeString(std::to_string(Helpers::JSON::ASDOUBLE_D(value, 0.0))))
            {
                writeStatus.succeed = false;
                return false;
            }
        }
        else if (value.is_bool())
        {
            if (!output->writeString(Helpers::JSON::ASBOOL_D(value, false) ? "true" : "false"))
            {
                writeStatus.succeed = false;
                return false;
            }
        }
        else
        {
            // Complex type (array/object) -> toJSON then HTML escape
            if (!output->writeString(Helpers::Encoders::htmlEscape(Helpers::JSON::toString(value))))
            {
                writeStatus.succeed = false;
                return false;
            }
        }
    }
    else if (action == "print_raw" && !value.is_null())
    {
        // Output raw string representation of the value
        if (!output->writeString(Helpers::JSON::ASSTRING_D(value, "")))
        {
            writeStatus.succeed = false;
            return false;
        }
    }
    return true;
}

void MantidsLang::processTokens(const boost::json::value &currentJsonContext, size_t depth)
{
    //const std::string indent(depth * 4, ' ');

    for (auto &token : tokens)
    {
        if (!writeStatus.succeed)
        {
            return;
        }

        if (token.type == Token::Type::SUBTAG && token.subTag)
        {
            // SUBTAG - recursively process (testing mode):
            //output->strPrintf("%s[%p-SUBTAG: %s]\n", indent.c_str(), static_cast<void *>(this), token.tagName.c_str());
            if (token.subTag->selfClosing)
            {
                // Parse tagName: "action:[path]"
                size_t bracketPos = token.tagName.find('[');
                if (bracketPos == std::string::npos)
                {
                    // Next Token
                    continue;
                }

                // Eg. {{#print_json[$session.user]!}}
                // Then: action = print_json
                // and parameter: session.user
                std::string action = token.tagName.substr(0, bracketPos);
                size_t endBracket = token.tagName.rfind(']');
                if (endBracket == std::string::npos || endBracket <= bracketPos)
                {
                    // Next Token
                    continue;
                }

                std::string parameter = token.tagName.substr(bracketPos + 1, endBracket - bracketPos - 1);
                boost::trim(parameter);

                if (boost::starts_with(action, "print_"))
                {
                    // Resolve JSON path

                    if (!boost::starts_with(parameter,"$"))
                    {
                        // Parameter should be in format: $json.path
                        continue;
                    }
                    parameter = parameter.substr(1);

                    Helpers::JSON::Path path(parameter);
                    const boost::json::value *valuePtr = path.resolve(currentJsonContext);
                    boost::json::value value = (valuePtr != nullptr) ? *valuePtr : boost::json::value{};
                    if (!printJSON(action, value))
                    {
                        return;
                    }
                }
                else if (action == "include_raw" || action == "render")
                {
                    std::shared_ptr<Memory::Streams::StreamableFile> file = std::make_shared<Memory::Streams::StreamableFile>();

                    std::string path =  defaultPath + parameter;

                    if (file->open(path.c_str(), O_RDONLY, 0)==-1)
                    {
                        // TODO: report..
                        continue;
                    }

                    if (action == "include_raw")
                    {
                        if (!file->streamTo(output))
                        {
                            return;
                        }
                    }
                    else if (action == "render")
                    {
                        MantidsLang parser(file, jsonContext, nullptr, apiCallback);
                        if (!parser.streamTo(output))
                        {
                            return;
                        }
                    }
                }
            }
            else
            {
                // Obtain action (first word before space or tab)
                size_t spacePos = token.tagName.find_first_of(" \t");
                std::string actionStr = (spacePos != std::string::npos) ? token.tagName.substr(0, spacePos) : token.tagName;
                if (actionStr == "foreach")
                {
                    /*
                     *  Usage syntax:
                       {{#foreach [session.scopes as scope]}}
                           {{#print_raw[scope]!}}
                       {{/foreach}}
                    */
                    if (!iterateJSON(token.tagName, currentJsonContext, token, depth))
                    {
                        return;
                    }
                }

                /*
                     *  Usage syntax:
                       {{#if [session.user != "admin"]}}
                           {{#print_raw[session.user]!}}
                       {{/if}}
                    */

                if (actionStr == "if")
                {
                    //Scripts_JSONExprEval
                    if (!checkConditionalOnJSON(token.tagName, currentJsonContext, token, depth))
                    {
                        return;
                    }
                }
            }
        }
        else
        {
            // DATA - write buffer to output
            if (!token.buffer.empty())
            {
                if (!output->writeFullStream(token.buffer.data(), token.buffer.size()))
                {
                    writeStatus.succeed = false;
                    return;
                }
            }
        }
    }
}