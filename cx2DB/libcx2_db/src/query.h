#ifndef QUERY_H
#define QUERY_H

#include <cx2_mem_vars/a_var.h>
#include <map>
#include <list>
#include <vector>
#include <mutex>
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

    // Internal functions (don't use):
    void setSqlConnector(void *value, std::mutex * mtDatabaseLock);

    // Query Prepare:
    bool setPreparedSQLQuery(const std::string &value, const std::map<std::string,Memory::Abstract::Var> & vars = {} );
    bool bindInputVars(const std::map<std::string,Memory::Abstract::Var> & vars);
    bool bindResultVars(const std::vector<Memory::Abstract::Var *> & vars);   
    bool getFetchLastInsertRowID() const;
    void setFetchLastInsertRowID(bool value);

    // TODO:
    // bool enqueue( void (*_callback)(Query *) = nullptr );

    // Query Execution:
    virtual bool exec(const ExecType & execType) = 0;

    // SELECT Results:
    virtual bool step() = 0;
    bool getIsNull(const size_t & column);
    unsigned long long getLastInsertRowID() const;

    // Error Management:
    std::string getLastSQLError() const;
    int getLastSQLReturnValue() const;

protected:
    virtual bool postBindInputVars() { return true; }
    virtual bool postBindResultVars() { return true; }

    bool replaceFirstKey(std::string &sqlQuery, std::list<std::string> &keysIn, std::vector<std::string> &keysOutByPos, const std::string replaceBy);

    std::string *createDestroyableString(const std::string &str);

    // Query:
    bool bBindInputVars, bBindResultVars;
    std::map<std::string,Memory::Abstract::Var> InputVars;
    std::string query;
    bool bFetchLastInsertRowID;

    // Internals:
    void * sqlConnector;

    // Errors:
    std::string lastSQLError;
    int lastSQLReturnValue;

    // Results:
    std::vector<bool> isNull;
    std::vector<Memory::Abstract::Var *> resultVars;
    unsigned long long lastInsertRowID;
    std::mutex * mtDatabaseLock;

private:
    // Memory cleaning:
    std::list<std::string *> destroyableStrings;

};

}}

#endif // QUERY_H
