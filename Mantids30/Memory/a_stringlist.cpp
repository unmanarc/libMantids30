#include "a_stringlist.h"
#include <Mantids30/DataFormat_JSON/json.h>
#include <mutex>
#include <string>

using namespace Mantids30::Memory::Abstract;

STRINGLIST::STRINGLIST()
{
    setVarType(Type::STRINGLIST);
}

STRINGLIST::STRINGLIST(const std::list<std::string> &value)
{
    setVarType(Type::STRINGLIST);
    this->m_value = value;
}

std::list<std::string> STRINGLIST::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_value;
}

bool STRINGLIST::setValue(const std::list<std::string> &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
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
        else
        { // If not, introduce the element.
            result += element;
        }
    }

    return result;
}

bool STRINGLIST::fromString(std::string_view inputStringList)
{
    std::list<std::string> finalValues;
    std::string::size_type currentPosition = 0, startOfCurrentField = 0;
    bool inQuotes = false, processWithoutQuotes = false;
    std::string currentFieldValue;

    while (currentPosition < inputStringList.size())
    {
        currentFieldValue.clear();

        if (!inQuotes)
        {
            if (!processWithoutQuotes && inputStringList.at(currentPosition) == '"')
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
                std::string::size_type commaPosition = inputStringList.find_first_of(',', currentPosition);
                // Last word....
                if (commaPosition != std::string::npos)
                {
                    // Others fields left...
                    currentFieldValue = inputStringList.substr(currentPosition, commaPosition - currentPosition);
                    finalValues.push_back(currentFieldValue);
                    currentPosition = commaPosition + 1;
                }
                else
                {
                    // End of string Field (not any other comma)...
                    currentFieldValue = inputStringList.substr(currentPosition, std::string::npos);
                    finalValues.push_back(currentFieldValue);
                    //currentPosition+=currentFieldValue.size();
                    break;
                }
            }
        }
        else
        {
            // In Quotes...
            while (currentPosition < inputStringList.size())
            {
                char currentChar = inputStringList.at(currentPosition);
                if (currentChar == '\"')
                {
                    if (currentPosition + 1 == inputStringList.size())
                    {
                        // End of String (not any other comma)...
                        finalValues.push_back(currentFieldValue);
                        currentPosition++;
                        currentFieldValue.clear();
                        inQuotes = false;
                        break;
                    }
                    else if (inputStringList.at(currentPosition + 1) == ',')
                    {
                        // Field completed on ", now take the field and push into the values.
                        // Process this field...
                        finalValues.push_back(currentFieldValue);
                        currentFieldValue.clear();
                        inQuotes = false;
                        currentPosition += 2;
                        startOfCurrentField = currentPosition;
                    }
                    else if (inputStringList.at(currentPosition + 1) == '\"' && currentPosition + 2 != inputStringList.size())
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
    std::shared_ptr<STRINGLIST> var = std::make_shared<STRINGLIST>();
    if (var)
    {
        *var = getValue();
    }
    return var;
}

boost::json::value STRINGLIST::toJSON()
{
    if (isNull())
    {
        return boost::json::value{};
    }

    std::shared_lock<std::shared_mutex> lock(m_mutex);
    boost::json::array j;
    for (const std::string &item : m_value)
    {
        j.emplace_back(item);
    }
    return j;
}

bool STRINGLIST::fromJSON(const boost::json::value &value)
{
    if (!value.is_array())
    {
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value.clear();
    for (const boost::json::value &item : value.as_array())
    {
        if (item.is_string())
        {
            m_value.emplace_back(Helpers::JSON::ASSTRING_D(item, ""));
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
