#ifndef ABSTRACTVARS_H
#define ABSTRACTVARS_H

#include "a_var.h"
#include <map>
#include <list>
#include <mdz_thr_mutex/mutex_shared.h>

// TODO: limits...
namespace Mantids { namespace Memory { namespace Abstract {

class Map
{
public:
    Map();
    virtual ~Map();

    /**
     * @brief set Set the variable (as a list of variables / submap)
     * @param varName variable name.
     * @param vars variables values.
     */
    void set(const std::string & varName, Map * vars);

    void setFromString(const std::string & varName, Mantids::Memory::Abstract::Type varType, const std::string & str);
    /**
     * @brief set Set variable.
     *               The variable will be destroyed
     * @param varName variable name.
     * @param var variable value.
     */
    void set(const std::string & varName, Mantids::Memory::Abstract::Var * var);
    /**
     * @brief getAsString Get variable as string.
     * @param varName variable name.
     * @return string if found, or empty string if not.
     */
    std::string getAsString(const std::string & varName);
    /**
     * @brief rem remove variable.
     * @param varName variable name
     */
    void rem(const std::string & varName, bool lock = true);
    /**
     * @brief get Get variable as abstract
     * @param varName variable name.
     * @return nullptr if not found,  Abstract if found.
     */
    Mantids::Memory::Abstract::Var * get(const std::string & varName);

    Map * getSubMap(const std::string & varName);

    std::list<std::string> getVarKeys();
    std::list<std::string> getVarListKeys();

private:
    std::map<std::string, Mantids::Memory::Abstract::Var *> vars;
    std::map<std::string, Map *> varsSubMap;

    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // ABSTRACTVARS_H
