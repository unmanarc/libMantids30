#ifndef ABSTRACTVARS_H
#define ABSTRACTVARS_H

#include <cx2_mem_abstracts/abstract.h>
#include <map>
#include <list>

// TODO: limits...
namespace CX2 { namespace Memory { namespace Vars {

class Abstracts
{
public:
    Abstracts();
    virtual ~Abstracts();

    /**
     * @brief set Set the variable (as a list of variables)
     * @param varName variable name.
     * @param vars variables values.
     */
    void set(const std::string & varName, Abstracts * vars);

    void setFromString(const std::string & varName, CX2::Memory::Vars::AVarType varType, const std::string & str);
    /**
     * @brief set Set variable.
     *               The variable will be destroyed
     * @param varName variable name.
     * @param var variable value.
     */
    void set(const std::string & varName, CX2::Memory::Vars::Abstract * var);
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
    void rem(const std::string & varName);
    /**
     * @brief get Get variable as abstract
     * @param varName variable name.
     * @return nullptr if not found,  Abstract if found.
     */
    CX2::Memory::Vars::Abstract * get(const std::string & varName);

    Abstracts * getList(const std::string & varName);

    std::list<std::string> getVarKeys();
    std::list<std::string> getVarListKeys();

private:
    std::map<std::string, CX2::Memory::Vars::Abstract *> vars;
    std::map<std::string, Abstracts *> varsList;
};

}}}

#endif // ABSTRACTVARS_H
