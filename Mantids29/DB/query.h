#ifndef QUERY_H
#define QUERY_H

#include <Mantids29/Memory/a_var.h>
#include <map>
#include <list>
#include <vector>
#include <mutex>
#include <string>

namespace Mantids29 { namespace Database {

class Query
{
public:
    enum ExecType
    {
        EXEC_TYPE_SELECT,
        EXEC_TYPE_INSERT
    };

    Query();
    virtual ~Query();

    // Internal functions (don't use):
    bool setSqlConnector(void *value, std::timed_mutex * m_databaseLockMutex, const uint64_t & milliseconds);

    // Query Prepare:
    bool setPreparedSQLQuery(const std::string &value, const std::map<std::string,Memory::Abstract::Var *> & vars = {} );
    bool bindInputVars(const std::map<std::string, Memory::Abstract::Var *> &vars);
    bool bindResultVars(const std::vector<Memory::Abstract::Var *> & vars);   
    bool getFetchLastInsertRowID() const;
    void setFetchLastInsertRowID(bool value);

    // TODO:
    // bool enqueue( void (*_callback)(Query *) = nullptr );

    // Query Execution:
    bool exec(const ExecType & execType);

    // GET ROW FROM SELECT Results:
    bool step();

    bool isNull(const size_t & column);
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
    virtual bool exec0(const ExecType & execType, bool recursion)=0;
    virtual bool step0() = 0;

    virtual bool postBindInputVars() { return true; }
    virtual bool postBindResultVars() { return true; }

    bool replaceFirstKey(std::string &sqlQuery, std::list<std::string> &keysIn, std::vector<std::string> &keysOutByPos, const std::string replaceBy);

    std::string *createDestroyableStringForInput(const std::string &str);
    void clearDestroyableStringsForInput();

    std::string *createDestroyableStringForResults(const std::string &str);
    void clearDestroyableStringsForResults();

    // Query:
    bool m_bindInputVars, m_bindResultVars;
    std::map<std::string,Memory::Abstract::Var *> m_inputVars;
    std::string m_query;
    bool m_fetchLastInsertRowID;

    // Internals:
    void * m_pSQLConnector;

    // Errors:
    std::string m_lastSQLError;
    int m_lastSQLErrno;
    int m_lastSQLReturnValue;

    // Results:
    std::vector<bool> m_fieldIsNull;
    std::vector<Memory::Abstract::Var *> m_resultVars;
    unsigned long long m_lastInsertRowID;
    uint64_t m_numRows;
    uint64_t m_affectedRows;
    std::timed_mutex * m_databaseLockMutex;

private:
    // Memory cleaning:
    std::list<std::string *> m_destroyableStringsForInput, m_destroyableStringsForResults;

};

}}

#endif // QUERY_H
