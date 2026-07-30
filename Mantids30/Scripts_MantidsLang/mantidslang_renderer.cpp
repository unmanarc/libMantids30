#include "mantidslang.h"

#include <Mantids30/Memory/streamable_null.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>

using namespace Mantids30;
using namespace Mantids30::Scripts;

void MantidsLang::processTokens(size_t depth)
{
    const std::string indent(depth * 4, ' ');

    for (auto &token : tokens)
    {
        if (!writeStatus.succeed)
        {
            return;
        }

        if (token.type == Token::Type::SUBTAG && token.subTag)
        {
            // SUBTAG - recursively process (testing mode):
            output->strPrintf("%s[%p-SUBTAG: %s]\n", indent.c_str(), static_cast<void *>(this), token.tagName.c_str());
            token.subTag->processTokens(depth + 1);
        }
        else
        {
            // DATA - write buffer to output
            if (!token.buffer.empty())
            {
                /*  Keep this way for testing purposes.
                 *  if (!output->writeFullStream(token.buffer.data(), token.buffer.size()))
                 *  {
                 *      writeStatus.succeed = false;
                 *      return;
                 *  }
                 */
                std::string x(token.buffer.data(), token.buffer.size());
                boost::replace_all(x, "\n", "\\n");
                output->strPrintf("%s[%p-DATA: %s]\n", indent.c_str(), static_cast<void *>(this), x.c_str());
            }
        }
    }
}