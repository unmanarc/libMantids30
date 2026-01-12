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
    Vars();
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
    T getTValue(const std::string& varName)
    {
        auto value = getValue(varName);
        if (value)
        {
            std::istringstream iss(value->toString());
            T result;
            iss >> result;
            if (iss.fail()|| !iss.eof())
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
    T getTValue(const std::string& varName, const T & defaultValue)
    {
        auto value = getValue(varName);
        if (value)
        {
            std::istringstream iss(value->toString());
            T result;
            iss >> result;
            if (iss.fail()|| !iss.eof())
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
    std::list<T> getTValues(const std::string& varName)
    {
        std::list<T> resultList;
        auto contList = getValues(varName);

        for (const auto &value : contList)
        {
            std::istringstream iss(value->toString());
            T valueResult;
            iss >> valueResult;

            if (!iss.fail()|| !iss.eof()) {
                // Only add to the list if the conversion succeeded
                resultList.push_back(valueResult);
            } else {
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
    bool exist(const std::string & varName);

    std::string getStringValue( const std::string & varName );
    ///////////////////////////////////////
    // Virtuals...
    /**
     * @brief varCount Get number of values that are registered to a variable name
     * @param varName Variable Name
     * @return number of values
     */
    virtual uint32_t varCount(const std::string & varName) = 0;
    /**
     * @brief getValue Get the first Memory Container for a variable name
     * @param varName variable name
     * @return first memory container abstract
     */
    virtual std::shared_ptr<Memory::Streams::StreamableObject>  getValue(const std::string & varName) = 0;
    /**
     * @brief getValues Get all the memory containers associated with a variable name
     * @param varName variable name
     * @return list of memory containers abstracts
     */
    virtual std::list<std::shared_ptr<Memory::Streams::StreamableObject> > getValues(const std::string & varName) = 0;
    /**
     * @brief getKeysList Get a set of all the registered variable names
     * @return set of all registered variable names
     */
    virtual std::set<std::string> getKeysList()=0;
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
    uint32_t getMaxVarNameSize() const;
    /**
     * @brief setMaxVarNameSize Set the maximum size for a variable name
     * @param value maximum size for a variable name
     */
    void setMaxVarNameSize(const uint32_t &value);

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
    virtual bool addVar(const std::string &varName, std::shared_ptr<Memory::Containers::B_Chunks> data) = 0;

protected:
    virtual void iSetMaxVarContentSize();
    virtual void iSetMaxVarNameSize();

    // Security options.
    uint32_t m_maxVarNameSize;
    size_t m_maxVarContentSize;
};

}

