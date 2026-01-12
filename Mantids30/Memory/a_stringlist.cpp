#include "a_stringlist.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

STRINGLIST::STRINGLIST()
{
    setVarType(TYPE_STRINGLIST);
}

STRINGLIST::STRINGLIST(const std::list<std::string> &value)
{
    setVarType(TYPE_STRINGLIST);
    this->m_value = value;
}

std::list<std::string> STRINGLIST::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return m_value;
}

bool STRINGLIST::setValue(const std::list<std::string> &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    this->m_value = value;
    return true;
}

std::string STRINGLIST::toString()
{
    std::list<std::string> values = getValue();
    std::string result;
    bool isFirst = true;

    for (const std::string &element : values)
    {
        // Check if double quotes are needed
        bool needsQuotes = (element.find(',') != std::string::npos || element.find('"') != std::string::npos);

        // If it's not the first element, add comma
        if (!isFirst)
        {
            result += ",";
        }
        else
        {
            isFirst = false;
        }

        // If quotes are needed...
        if (needsQuotes)
        {
            // Encapsulate the field
            result += '"';
            // Escape double quotes if needed
            for (char c : element)
            {
                if (c == '"')
                {
                    result += "\"\"";
                }
                else
                {
                    result += c;
                }
            }
            // Close the encapsulation
            result += '"';
        }
        else // If not, introduce the element.
            result += element;
    }

    return result;
}

bool STRINGLIST::fromString(const std::string &inputString)
{
    std::list<std::string> finalValues;
    std::string::size_type currentPosition = 0, startOfCurrentField = 0;
    bool inQuotes = false, processWithoutQuotes = false;
    std::string currentFieldValue;

    while (currentPosition < inputString.size())
    {
        currentFieldValue = "";

        if (!inQuotes)
        {
            if (!processWithoutQuotes && inputString.at(currentPosition) == '"')
            {
                // Inside Quotes begin...
                inQuotes = true;
                startOfCurrentField = currentPosition;
                currentPosition++;
            }
            else
            {
                // if processWithoutQuotes is true, change to false.
                processWithoutQuotes = false;

                // Not Inside Quotes... find the coma value and put the current value...
                std::string::size_type commaPosition = inputString.find_first_of(',', currentPosition);
                // Last word....
                if (commaPosition != std::string::npos)
                {
                    // Others fields left...
                    currentFieldValue = inputString.substr(currentPosition, commaPosition - currentPosition);
                    finalValues.push_back(currentFieldValue);
                    currentPosition = commaPosition + 1;
                }
                else
                {
                    // End of string Field (not any other comma)...
                    currentFieldValue = inputString.substr(currentPosition, std::string::npos);
                    finalValues.push_back(currentFieldValue);
                    //currentPosition+=currentFieldValue.size();
                    break;
                }
            }
        }
        else
        {
            // In Quotes...
            while (currentPosition < inputString.size())
            {
                char currentChar = inputString.at(currentPosition);
                if (currentChar == '\"')
                {
                    if (currentPosition + 1 == inputString.size())
                    {
                        // End of String (not any other comma)...
                        finalValues.push_back(currentFieldValue);
                        currentPosition++;
                        currentFieldValue = "";
                        inQuotes = false;
                        break;
                    }
                    else if (inputString.at(currentPosition + 1) == ',')
                    {
                        // Field completed on ", now take the field and push into the values.
                        // Process this field...
                        finalValues.push_back(currentFieldValue);
                        currentFieldValue = "";
                        inQuotes = false;
                        currentPosition += 2;
                        startOfCurrentField = currentPosition;
                    }
                    else if (inputString.at(currentPosition + 1) == '\"' && currentPosition + 2 != inputString.size())
                    {
                        // Double Quote... take only one..
                        currentFieldValue += currentChar;
                        currentPosition += 2;
                    }
                    else
                    {
                        // 1. the " is followed by unknown bytes... revert the escaping?
                        // 2. the "" is followed by the end of the string... revert the escaping?
                        currentFieldValue = "IGN";
                        break;
                    }
                }
                else
                {
                    currentFieldValue += currentChar;
                    currentPosition++;
                }
            }

            if (!currentFieldValue.empty())
            {
                // the processing var was not completed. Revert the current Position...
                currentPosition = startOfCurrentField;
                processWithoutQuotes = true;
                inQuotes = false;
            }
        }
    }
    return setValue(finalValues);
}

std::shared_ptr<Var> STRINGLIST::protectedCopy()
{
    auto var = std::make_shared<STRINGLIST>();
    if (var)
        *var = getValue();
    return var;
}

Json::Value STRINGLIST::toJSON()
{
    if (isNull())
        return Json::nullValue;

    Threads::Sync::Lock_RD lock(m_mutex);
    Json::Value j(Json::arrayValue);
    for (const auto &item : m_value)
    {
        j.append(item);
    }
    return j;
}

bool STRINGLIST::fromJSON(const Json::Value &value)
{
    if (!value.isArray())
        return false;

    Threads::Sync::Lock_RW lock(m_mutex);
    m_value.clear();
    for (const auto &item : value)
    {
        if (item.isString())
        {
            m_value.push_back(JSON_ASSTRING_D(item, ""));
        }
        else
        {
            // If any element is not a string, we can either skip it or return false
            // For now, let's assume all elements must be strings and return false if not.
            return false;
        }
    }
    return true;
}
