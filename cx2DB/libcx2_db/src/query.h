#ifndef QUERY_H
#define QUERY_H

#include <cx2_mem_vars/a_var.h>
#include <map>
#include <list>
#include <string>

namespace CX2 { namespace Database {

enum ExecType
{
    EXEC_TYPE_SELECT,
    EXEC_TYPE_INSERT
};

class Query
{
public:
    Query();
    virtual ~Query();

    void setSqlConnector(void *value);

    void setPreparedSQLQuery(const std::string &value, const std::map<std::string,Memory::Abstract::Var> & vars = {} );

    bool bindInputVars(const std::map<std::string,Memory::Abstract::Var> & vars);
    bool bindResultVars(const std::list<Memory::Abstract::Var *> & vars);

    // TODO:
  //  bool enqueue( void (*_callback)(Query *) = nullptr );

    virtual bool exec(const ExecType & execType) = 0;
    virtual bool step() = 0;

    std::string getLastSQLError() const;

    int getLastSQLReturnValue() const;

protected:
    virtual bool postBindInputVars() { return true; }
    virtual bool postBindResultVars() { return true; }

    std::list<Memory::Abstract::Var *> resultVars;
    std::map<std::string,Memory::Abstract::Var> InputVars;
    std::string query;

    bool bBindInputVars, bBindResultVars;
    void * sqlConnector;
    std::string lastSQLError;
    int lastSQLReturnValue;
};

}}

#endif // QUERY_H
