#pragma once

#include "a_var.h"
#include <map>
#include <list>
#include <Mantids30/Threads/mutex_shared.h>
#include <memory>

// TODO: limits...
namespace Mantids30 { namespace Memory { namespace Abstract {


class VariableMap
{
public:
    VariableMap();
    virtual ~VariableMap();

    /**
     * @brief Insert or update a submap in the variable map.
     * @param variableName The name of the variable.
     * @param submap A shared pointer to the submap.
     */
    void insertOrUpdateSubmap(const std::string &variableName, std::shared_ptr<VariableMap> submap);

    /**
     * @brief Create or update a variable from a string representation based on its type.
     * @param variableName The name of the variable.
     * @param variableType The type of the variable as defined in Var::Type.
     * @param valueString The string representation of the variable's value.
     */
    void setVariableFromString(const std::string &variableName, Mantids30::Memory::Abstract::Var::Type variableType, const std::string &valueString);

    /**
     * @brief Insert or update a variable in the map.
     * @param variableName The name of the variable.
     * @param variable A shared pointer to the variable.
     */
    void insertOrUpdateVariable(const std::string &variableName, std::shared_ptr<Var> variable);

    /**
     * @brief Retrieve the string representation of a variable.
     * @param variableName The name of the variable.
     * @return The string representation if found, otherwise an empty string.
     */
    std::string getVariableAsString(const std::string &variableName);

    /**
     * @brief Remove a variable from the map.
     * @param variableName The name of the variable.
     * @param lock Optional parameter to lock the operation.
     */
    void removeVariable(const std::string &variableName, bool lock = true);

    /**
     * @brief Retrieve a variable as an abstract type.
     * @param variableName The name of the variable.
     * @return A shared pointer to the variable if found, otherwise nullptr.
     */
    std::shared_ptr<Mantids30::Memory::Abstract::Var> getVariable(const std::string &variableName);

    /**
     * @brief Retrieve a submap from the map.
     * @param variableName The name of the variable that maps to the submap.
     * @return A shared pointer to the submap if found, otherwise nullptr.
     */
    std::shared_ptr<VariableMap> getSubmap(const std::string &variableName);

    /**
     * @brief List all keys of variables stored in the map.
     * @return A list of variable names.
     */
    std::list<std::string> listVariableKeys();

    /**
     * @brief List all keys that are mapped to submaps.
     * @return A list of keys that are submaps.
     */
    std::list<std::string> listSubmapKeys();

private:
    std::map<std::string, std::shared_ptr<Mantids30::Memory::Abstract::Var>> m_variables;
    std::map<std::string, std::shared_ptr<VariableMap>> m_submaps;

    Threads::Sync::Mutex_Shared m_mutex;
};

}}}

