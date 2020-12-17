#include "query.h"
#include "sqlconnector.h"
#include <stdexcept>

using namespace CX2::Database;

Query::Query()
{
    lastSQLReturnValue = 0;
    sqlConnector = nullptr;
    bBindInputVars = false;
    bBindResultVars = false;
}

Query::~Query()
{
    if (sqlConnector)
    {
        ((SQLConnector *)sqlConnector)->detachQuery(this);
    }
}

void Query::setPreparedSQLQuery(const std::string &value, const std::map<std::string, CX2::Memory::Abstract::Var> &vars)
{
    bindInputVars(vars);
    query = value;
}

bool Query::bindInputVars(const std::map<std::string, CX2::Memory::Abstract::Var> &vars)
{
    if (bBindInputVars)
        throw std::runtime_error("Don't call bindInputVars twice.");
    bBindInputVars = true;
    InputVars = vars;
    return postBindInputVars();
}

bool Query::bindResultVars(const std::list<CX2::Memory::Abstract::Var *> &vars)
{
    if (bBindResultVars)
        throw std::runtime_error("Don't call bindResultVars twice.");
    bBindResultVars = true;
    resultVars = vars;
    return postBindResultVars();
}

std::string Query::getLastSQLError() const
{
    return lastSQLError;
}

int Query::getLastSQLReturnValue() const
{
    return lastSQLReturnValue;
}

void Query::setSqlConnector(void *value)
{
    sqlConnector = value;
}
