#ifndef QUERY_H
#define QUERY_H

#include <mdz_mem_vars/a_var.h>
#include <map>
#include <list>
#include <vector>
#include <mutex>
#include <string>

namespace Mantids { namespace Database {

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
    bool setPreparedSQLQuery(const std::string &value, const std::map<std::string,Memory::Abstract::Var *> & vars = {} );
    bool bindInputVars(const std::map<std::string, Memory::Abstract::Var *> &vars);
    bool bindResultVars(const std::vector<Memory::Abstract::Var *> & vars);   
    bool getFetchLastInsertRowID() const;
    void setFetchLastInsertRowID(bool value);

    // TODO:
    // bool enqueue( void (*_callback)(Query *) = nullptr );

    // Query Execution:
    virtual bool exec(const ExecType & execType) = 0;

    // GET ROW FROM SELECT Results:
    bool step();

    bool getIsNull(const size_t & column);
    unsigned long long getLastInsertRowID() const;

    // Error Management:
    std::string getLastSQLError() const;
    int getLastSQLReturnValue() const;

    /**
     * @brief getNumRows Get the retrieved rows count on SELECT statement
     * warning: does not apply to sqlite3
     * @return number of rows retrieved by the select statement
     */
    uint64_t getNumRows() const;
    /**
     * @brief getAffectedRows Get Affected Rows by INSERT/UPDATE/DELETE commands
     * @return number of affected rows.
     */
    uint64_t getAffectedRows() const;

protected:
    virtual bool step0() = 0;

    virtual bool postBindInputVars() { return true; }
    virtual bool postBindResultVars() { return true; }

    bool replaceFirstKey(std::string &sqlQuery, std::list<std::string> &keysIn, std::vector<std::string> &keysOutByPos, const std::string replaceBy);

    std::string *createDestroyableStringForInput(const std::string &str);
    void clearDestroyableStringsForInput();

    std::string *createDestroyableStringForResults(const std::string &str);
    void clearDestroyableStringsForResults();

    // Query:
    bool bBindInputVars, bBindResultVars;
    std::map<std::string,Memory::Abstract::Var *> InputVars;
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
    uint64_t numRows;
    uint64_t affectedRows;
    std::mutex * mtDatabaseLock;

private:
    // Memory cleaning:
    std::list<std::string *> destroyableStringsForInput, destroyableStringsForResults;

};

}}

#endif // QUERY_H
