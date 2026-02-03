#pragma once

#include "b_chunks.h"
#include "streamable_object.h"
#include <Mantids30/Helpers/json.h>
#include <list>
#include <set>
#include <string>

namespace Mantids30::Memory::Abstract {

class Vars
{
public:
    Vars() = default;
    virtual ~Vars() = default;

    // Methods to convert to and from JSON
    json toJSON();
    bool fromJSON(const Json::Value &json);

    ///////////////////////////////////////
    // Generic conversion.
    /**
     * @brief getTValue Get the <template> value (converted from the binary container) for an specific variable
     * @param varName Variable name
     * @return T value
     */
    template<typename T>
    T getTValue(const std::string &varName)
    {
        auto value = getValue(varName);
        if (value)
        {
            std::istringstream iss(value->toString());
            T result;
            iss >> result;
            if (iss.fail() || !iss.eof())
            {
                // Conversion failed, return default value
                return T{}; // For numeric types, T{} will be 0
            }
            return result;
        }
        // Variable not found, return default value
        return T{};
    }

    template<typename T>
    T getTValue(const std::string &varName, const T &defaultValue)
    {
        auto value = getValue(varName);
        if (value)
        {
            std::istringstream iss(value->toString());
            T result;
            iss >> result;
            if (iss.fail() || !iss.eof())
            {
                // Conversion failed, return default value
                return defaultValue; // Parsing failed, return the default value
            }
            return result;
        }
        // Variable not found, return default value
        return defaultValue;
    }

    /**
     * @brief getTValues Get the all the T values (converted from the binary containers) for an specific variable
     * @param varName Variable Name
     * @return list of T values
     */
    template<typename T>
    std::list<T> getTValues(const std::string &varName)
    {
        std::list<T> resultList;
        auto contList = getValues(varName);

        for (const auto &value : contList)
        {
            std::istringstream iss(value->toString());
            T valueResult;
            iss >> valueResult;

            if (!iss.fail() || !iss.eof())
            {
                // Only add to the list if the conversion succeeded
                resultList.push_back(valueResult);
            }
            else
            {
                // If the conversion failed, don't add.
            }
        }

        return resultList;
    }

    ///////////////////////////////////////
    // Var Existence.
    /**
     * @brief exist Check if a variable exist
     * @param varName Variable Name
     * @return true if exist
     */
    bool exist(const std::string &varName);

    std::string getStringValue(const std::string &varName);
    ///////////////////////////////////////
    // Virtuals...
    /**
     * @brief varCount Get number of values that are registered to a variable name
     * @param varName Variable Name
     * @return number of values
     */
    virtual uint32_t varCount(const std::string &varName) = 0;
    /**
     * @brief getValue Get the first Memory Container for a variable name
     * @param varName variable name
     * @return first memory container abstract
     */
    virtual std::shared_ptr<Memory::Streams::StreamableObject> getValue(const std::string &varName) = 0;
    /**
     * @brief getValues Get all the memory containers associated with a variable name
     * @param varName variable name
     * @return list of memory containers abstracts
     */
    virtual std::list<std::shared_ptr<Memory::Streams::StreamableObject>> getValues(const std::string &varName) = 0;
    /**
     * @brief getKeysList Get a set of all the registered variable names
     * @return set of all registered variable names
     */
    virtual std::set<std::string> getKeysList() = 0;
    /**
     * @brief isEmpty Get if the variable container is empty
     * @return true if no variable is inserted
     */
    virtual bool isEmpty() = 0;

    ///////////////////////////////////////
    // Security options:
    /**
     * @brief getMaxVarNameSize Get the maximum size for a variable name
     * @return maximum size for a variable name
     */
    size_t getMaxVarNameSize() const;
    /**
     * @brief setMaxVarNameSize Set the maximum size for a variable name
     * @param value maximum size for a variable name
     */
    void setMaxVarNameSize(const size_t &value);

    /**
     * @brief getMaxVarContentSize Get the maximum size for a variable content value
     * @return maximum size for a variable content value
     */
    size_t getMaxVarContentSize() const;
    /**
     * @brief setMaxVarContentSize Set the maximum size for a variable content value
     * @param value maximum size for a variable content value
     */
    void setMaxVarContentSize(const size_t &value);

    virtual void clear() = 0;

    /**
     * @brief addVar Add Var
     * @param varName variable name
     * @param data data container
     * @return true if added, false otherwise.
     */
    virtual bool addVar(const std::string &varName, std::shared_ptr<Memory::Containers::B_Chunks> data) = 0;

    /**
     * @brief setMaxVarsCount Set Maxium Number of variables allowed
     * @param newMaxVarsCount max number of variables.
     */
    void setMaxVarsCount(size_t newMaxVarsCount);

protected:
    virtual void iSetMaxVarContentSize();
    virtual void iSetMaxVarNameSize();

    // Security limits.
    // These bounds protect against excessive memory usage and abuse.
    // They should be aligned with upstream protocol limits (e.g. HTTP max content size)
    // and adjusted carefully if relaxed.

    size_t m_maxVarNameSize    = 1 * KB_MULT;   // Maximum variable name length (1 KB)
    size_t m_maxVarContentSize = 16 * MB_MULT;  // Maximum content size per variable (16 MB)
    size_t m_maxVarsCount      = 512;           // Maximum number of variables allowed
};

} // namespace Mantids30::Memory::Abstract
