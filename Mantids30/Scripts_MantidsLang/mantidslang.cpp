#include "mantidslang.h"

#include <Mantids30/Memory/streamable_null.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <regex>

using namespace Mantids30;
using namespace Mantids30::Scripts;

namespace {

// {{ ... }} non greedy
const std::regex &tagRegex()
{
    static const std::regex r(R"(\{\{([^}]*?)\}\})");
    return r;
}

std::string trimCopy(const std::string &in)
{
    std::string out = in;
    boost::trim(out);
    return out;
}

// Extract the command name (first word) from a tag content
std::string extractCommandName(const std::string &tagName)
{
    std::string trimmed = trimCopy(tagName);
    const size_t spacePos = trimmed.find_first_of(" \t\r\n");
    if (spacePos != std::string::npos)
    {
        return trimmed.substr(0, spacePos);
    }
    return trimmed;
}

} // namespace

MantidsLang::MantidsLang(const std::shared_ptr<Memory::Streams::StreamableObject> &source, const std::shared_ptr<Json::Value> &jsonContext, MantidsLang *parent, ApiCallback_t apiCallback)
    : source(source)
    , jsonContext(jsonContext)
    , parent(parent)
    , apiCallback(std::move(apiCallback))
{
    // Initialize with one empty DATA token
    tokens.emplace_back();
}

bool MantidsLang::streamTo(Memory::Streams::StreamableObject *out)
{
    output = out;

    // Read the source into this object
    if (!source->streamTo(this))
    {
        return false;
    }

    // Send EOF to trigger final processing
    return writeEOF();
}

MantidsLang::Token &MantidsLang::currentDataToken()
{
    // Ensure we append to a DATA token, not a SUBTAG token
    if (tokens.empty() || tokens.back().type != Token::Type::DATA)
    {
        tokens.emplace_back();
    }
    return tokens.back();
}

std::optional<size_t> MantidsLang::write(const void *buf, const size_t &count)
{
    if (output == nullptr)
    {
        return std::nullopt;
    }

    // EOF signal (count == 0)
    if (count == 0)
    {
        if (!eofReached)
        {
            eofReached = true;

            // Flush/parse whatever is pending in this object and in the whole
            // chain of still-open subtags (unterminated tags at EOF).
            finalizeParsing();

            // Only the root owns the output dump; nested instances are walked
            // recursively from the root's processTokens().
            if (parent == nullptr)
            {
                processTokens(*jsonContext);
            }
        }
        return 0;
    }

    // This subtag was already closed: data belongs to the parent context.
    if (closed)
    {
        if (parent == nullptr)
        {
            return std::nullopt;
        }
        if (!parent->write(buf, count))
        {
            return std::nullopt;
        }
        return count;
    }

    // If inside an active subtag, delegate all writes to it.
    // NOTE: cache the pointer, because the subtag may close itself (and reset
    // this->activeSubTag) during the call.
    if (activeSubTag != nullptr)
    {
        MantidsLang *sub = activeSubTag;
        if (!sub->write(buf, count))
        {
            return std::nullopt;
        }
        return count;
    }

    // Append data to current last DATA token's buffer
    Token &dataToken = currentDataToken();
    const auto *p = static_cast<const char *>(buf);
    dataToken.buffer.insert(dataToken.buffer.end(), p, p + count);

    // Parse tags from the accumulated buffer
    parseTagsFromBuffer();

    return count;
}

bool MantidsLang::isEmpty() const { return tokens.empty(); }

void MantidsLang::finalizeParsing()
{
    // Depth-first: the innermost open context first.
    if (activeSubTag != nullptr)
    {
        activeSubTag->finalizeParsing();
        // The chain is unterminated; nothing else can be parsed here.
        return;
    }
    parseTagsFromBuffer();
}

void MantidsLang::parseTagsFromBuffer()
{
    // If inside a subtag, don't parse here
    if (activeSubTag != nullptr)
    {
        return;
    }

    // Ensure we are looking at a DATA token
    if (tokens.empty() || tokens.back().type != Token::Type::DATA)
    {
        return;
    }

    const std::vector<char> &buf = tokens.back().buffer;
    const std::string bufStr(buf.begin(), buf.end());

    // consumed  -> data already assigned to previous tokens
    // searchPos -> where to keep looking for tags (skipped/unknown tags move
    //              this forward WITHOUT consuming data)
    size_t consumed = 0;
    size_t searchPos = 0;
    bool bufferRewritten = false;

    std::smatch match;
    while (searchPos <= bufStr.size() && std::regex_search(bufStr.cbegin() + static_cast<long>(searchPos), bufStr.cend(), match, tagRegex()))
    {
        const size_t matchPos = searchPos + static_cast<size_t>(match.position(0));
        const size_t matchLen = static_cast<size_t>(match.length(0));
        const size_t afterMatch = matchPos + matchLen;

        const std::string tagContent = trimCopy(match[1].str());
        const std::string dataBefore = bufStr.substr(consumed, matchPos - consumed);

        if (!tagContent.empty() && tagContent[0] == '#')
        {
            std::string tagName = trimCopy(tagContent.substr(1));

            // Self-closing tag: {{#NAME!}}
            if (tagName.size() > 1 && tagName.back() == '!')
            {
                // Finalize current DATA token with content before tag
                tokens.back().type = Token::Type::DATA;
                tokens.back().buffer.assign(dataBefore.begin(), dataBefore.end());

                const std::string cleanName = trimCopy(tagName.substr(0, tagName.size() - 1));
                addSubTagToken(cleanName, false);

                // Start new DATA token for content after
                tokens.emplace_back();

                consumed = afterMatch;
                searchPos = afterMatch;
                bufferRewritten = true;
                continue;
            }

            // Opening tag: {{#NAME}}
            // Finalize current DATA token with content before tag
            tokens.back().type = Token::Type::DATA;
            tokens.back().buffer.assign(dataBefore.begin(), dataBefore.end());

            // Create subtag; this also appends the empty DATA token that will
            // receive whatever comes after the subtag is closed.
            createSubTag(tagName);

            // IMPORTANT: everything must be finalized BEFORE delegating, because
            // the subtag may close itself inside this call and write back into us.
            const std::string remaining = (afterMatch < bufStr.size()) ? bufStr.substr(afterMatch) : std::string();

            MantidsLang *sub = activeSubTag;
            if (sub != nullptr && !remaining.empty())
            {
                sub->write(remaining.c_str(), remaining.size());
            }
            return;
        }
        else if (!tagContent.empty() && tagContent[0] == '/')
        {
            const std::string tagName = trimCopy(tagContent.substr(1));

            // Is this our expected closing tag?
            if (!expectedClosingTag.empty() && expectedClosingTag == tagName)
            {
                // Finalize current DATA token with content before closing tag
                tokens.back().type = Token::Type::DATA;
                tokens.back().buffer.assign(dataBefore.begin(), dataBefore.end());

                const std::string dataAfterClosing = (afterMatch < bufStr.size()) ? bufStr.substr(afterMatch) : std::string();

                // Close this subtag and return control to the parent
                closeSubTag(dataAfterClosing);
                return;
            }

            // Not our closing tag: keep it as literal data, just skip it.
            searchPos = afterMatch;
            continue;
        }
        else
        {
            // Not a structural tag (expression/variable/unknown): keep as data.
            searchPos = afterMatch;
            continue;
        }
    }

    // No structural tag left: the tail (after the last consumed position) stays
    // in the current DATA token, waiting for more writes.
    if (bufferRewritten)
    {
        const std::string remainder = bufStr.substr(consumed);
        tokens.back().type = Token::Type::DATA;
        tokens.back().buffer.assign(remainder.begin(), remainder.end());
    }
}

void MantidsLang::addSubTagToken(const std::string &tagName, bool activate)
{
    Token subTagToken;
    subTagToken.type = Token::Type::SUBTAG;
    subTagToken.tagName = tagName;
    subTagToken.subTag = std::make_unique<MantidsLang>(nullptr, jsonContext, this, apiCallback);
    subTagToken.subTag->output = output;

    // FIX: it is the *opening* tag (activate==true) the one that has to look for
    // its own {{/NAME}}. A self-closing tag has no body at all.
    if (activate)
    {
        subTagToken.subTag->expectedClosingTag = extractCommandName(tagName);
    }
    else
    {
        subTagToken.subTag->closed = true;
    }

    tokens.push_back(std::move(subTagToken));
}

void MantidsLang::createSubTag(const std::string &tagName)
{
    addSubTagToken(tagName, true);

    // Now all writes go to this subtag
    activeSubTag = tokens.back().subTag.get();

    // Add a new DATA token so subsequent data after subtag closes has a place to go
    tokens.emplace_back();
}

/*
 * Called on the SUBTAG itself when it finds its matching {{/NAME}} closing tag.
 * Its last DATA token has already been finalized with the content before the
 * closing tag; here we give control back to the parent and hand over the tail.
 */
void MantidsLang::closeSubTag(const std::string &dataAfterClosing)
{
    closed = true;

    // Remove our own trailing empty DATA token (added by createSubTag for our
    // last nested subtag) so the dump has no empty entries.
    if (tokens.size() > 1 && tokens.back().type == Token::Type::DATA && tokens.back().buffer.empty())
    {
        tokens.pop_back();
    }

    // FIX: the pointer that must be cleared is the PARENT's one; clearing our
    // own did nothing and the parent kept funnelling every write into us.
    if (parent == nullptr)
    {
        return;
    }
    if (parent->activeSubTag == this)
    {
        parent->activeSubTag = nullptr;
    }

    // Pass remaining data to parent (it now lands in the parent's DATA token)
    if (!dataAfterClosing.empty())
    {
        parent->write(dataAfterClosing.c_str(), dataAfterClosing.size());
    }
}
