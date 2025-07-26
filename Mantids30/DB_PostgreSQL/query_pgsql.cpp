#include "query_pgsql.h"
#include <memory>
#include <stdexcept>
#include "sqlconnector_pgsql.h"
#include <string.h>
#include <Mantids30/Memory/a_allvars.h>

#include <stdexcept>

using namespace Mantids30::Database;

Query_PostgreSQL::Query_PostgreSQL()
{
    m_results = nullptr;
    m_paramCount = 0;
    m_currentRow = 0;

    m_paramValues=nullptr;
    m_paramLengths=nullptr;
    m_paramFormats=nullptr;

    m_databaseConnectionHandler = nullptr;
    m_results = nullptr;
}

Query_PostgreSQL::~Query_PostgreSQL()
{
    if (m_results) PQclear(m_results);
    m_results = nullptr;

    free(m_paramValues);
    free(m_paramLengths);
    free(m_paramFormats);
}

// TODO: lastSQLError

bool Query_PostgreSQL::step0()
{
    //  :)
    if (!m_results) 
        return false;
    if (m_execStatus != PGRES_TUPLES_OK) 
        return false;

    int i = m_currentRow;

    if (m_currentRow >= PQntuples(m_results)) 
        return false;

    int columnpos = 0;
    for ( const auto &outputVar : m_resultVars)
    {
        m_fieldIsNull.push_back(PQgetisnull(m_results,i,columnpos));

        switch (outputVar->getVarType())
        {
        case Memory::Abstract::Var::TYPE_BOOL:
            ABSTRACT_PTR_AS(BOOL,outputVar)->fromString(PQgetvalue(m_results,i,columnpos));
            break;
        case Memory::Abstract::Var::TYPE_INT8:
            ABSTRACT_PTR_AS(INT8,outputVar)->fromString(PQgetvalue(m_results,i,columnpos));
            break;
        case Memory::Abstract::Var::TYPE_INT16:
            ABSTRACT_PTR_AS(INT16,outputVar)->fromString(PQgetvalue(m_results,i,columnpos));
            break;
        case Memory::Abstract::Var::TYPE_INT32:
            ABSTRACT_PTR_AS(INT32,outputVar)->fromString(PQgetvalue(m_results,i,columnpos));
            break;
        case Memory::Abstract::Var::TYPE_INT64:
            ABSTRACT_PTR_AS(INT64,outputVar)->fromString(PQgetvalue(m_results,i,columnpos));
            break;
        case Memory::Abstract::Var::TYPE_UINT8:
            ABSTRACT_PTR_AS(UINT8,outputVar)->fromString(PQgetvalue(m_results,i,columnpos));
            break;
        case Memory::Abstract::Var::TYPE_UINT16:
            ABSTRACT_PTR_AS(UINT16,outputVar)->fromString(PQgetvalue(m_results,i,columnpos));
            break;
        case Memory::Abstract::Var::TYPE_UINT32:
            ABSTRACT_PTR_AS(UINT32,outputVar)->fromString(PQgetvalue(m_results,i,columnpos));
            break;
        case Memory::Abstract::Var::TYPE_UINT64:
            ABSTRACT_PTR_AS(UINT64,outputVar)->fromString(PQgetvalue(m_results,i,columnpos));
            break;
        case Memory::Abstract::Var::TYPE_DOUBLE:
            ABSTRACT_PTR_AS(DOUBLE,outputVar)->fromString(PQgetvalue(m_results,i,columnpos));
            break;
        case Memory::Abstract::Var::TYPE_BIN:
        {
            Memory::Abstract::BINARY::sBinContainer binContainer;
            binContainer.ptr = (char *)PQgetvalue(m_results,i,columnpos);
            // TODO: should bytes need to be 64-bit for blob64?
            binContainer.dataSize = PQgetlength(m_results,i,columnpos);
            ABSTRACT_PTR_AS(BINARY,outputVar)->setValue( &binContainer );
            binContainer.ptr = nullptr; // don't destroy the data.
        } break;
        case Memory::Abstract::Var::TYPE_VARCHAR:
        {
            // This will copy the memory.
            ABSTRACT_PTR_AS(VARCHAR,outputVar)->setValue( PQgetvalue(m_results,i,columnpos) );
        } break;
        case Memory::Abstract::Var::TYPE_STRING:
        {
            ABSTRACT_PTR_AS(STRING,outputVar)->setValue( PQgetvalue(m_results,i,columnpos) );
        }break;
        case Memory::Abstract::Var::TYPE_STRINGLIST:
        {
            ABSTRACT_PTR_AS(STRINGLIST,outputVar)->fromString( PQgetvalue(m_results,i,columnpos) );
        }break;
        case Memory::Abstract::Var::TYPE_DATETIME:
        {
            ABSTRACT_PTR_AS(DATETIME,outputVar)->fromString( PQgetvalue(m_results,i,columnpos) );
        }break;
        case Memory::Abstract::Var::TYPE_IPV4:
        {
            ABSTRACT_PTR_AS(IPV4,outputVar)->fromString( PQgetvalue(m_results,i,columnpos) );
        }break;
        case Memory::Abstract::Var::TYPE_MACADDR:
        {
            ABSTRACT_PTR_AS(MACADDR,outputVar)->fromString( PQgetvalue(m_results,i,columnpos) );
        }break;
        case Memory::Abstract::Var::TYPE_IPV6:
        {
            ABSTRACT_PTR_AS(IPV6,outputVar)->fromString( PQgetvalue(m_results,i,columnpos) );
        }break;
        case Memory::Abstract::Var::TYPE_PTR:
        {
            // This will reference the memory, but will disappear on the next step
            ABSTRACT_PTR_AS(PTR,outputVar)->setValue( PQgetvalue(m_results,i,columnpos) );
        } break;
        case Memory::Abstract::Var::TYPE_NULL:
            // Don't copy the value (not needed).
            break;
        }

        columnpos++;
    }

    m_currentRow++;

    return false;
}

void Query_PostgreSQL::psqlSetDatabaseConnector(PGconn *conn)
{
    this->m_databaseConnectionHandler = conn;
}

ExecStatusType Query_PostgreSQL::psqlGetExecStatus() const
{
    return m_execStatus;
}

bool Query_PostgreSQL::postBindInputVars()
{
    m_paramCount = 0;

    std::list<std::string> keysIn;
    for (auto & i : m_inputVars) keysIn.push_back(i.first);

    // Replace the named keys for $0, $1, etc...:
    while (replaceFirstKey(m_query,keysIn,m_keysByPos, std::string("$") + std::to_string(m_paramCount)))
    {
        m_paramCount++;
    }

    if (m_paramCount!=m_keysByPos.size())
    {
        throw std::runtime_error("Param count is not the same size of keys by pos (please report).");
    }

    if (m_paramValues)
    {
        throw std::runtime_error("Can't bind input variables twice (please report).");
    }

    m_paramValues = static_cast<char **>( malloc (m_paramCount * sizeof(char *)) );
    m_paramLengths = static_cast<int *>(malloc( m_paramCount * sizeof(int)) );
    m_paramFormats = static_cast<int *>(malloc( m_paramCount * sizeof(int)) );

    for (size_t pos=0; pos<m_keysByPos.size(); pos++)
    {
        std::shared_ptr<std::string> str = nullptr;
        m_paramFormats[pos] = 0;
        std::string key = m_keysByPos[pos];

        /*
        Bind params here.
        */
        switch (m_inputVars[ key ]->getVarType())
        {
        case Memory::Abstract::Var::TYPE_BOOL:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(BOOL,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_INT8:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(INT8,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_INT16:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(INT16,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_INT32:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(INT32,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_INT64:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(INT64,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_UINT8:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(UINT8,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_UINT16:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(UINT16,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_UINT32:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(UINT32,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_UINT64:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(UINT64,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_DATETIME:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(DATETIME,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_DOUBLE:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(DOUBLE,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_BIN:
        {
            auto * i =ABSTRACT_SPTR_AS(BINARY,m_inputVars[key])->getValue();
            m_paramValues[pos] = i->ptr;
            m_paramLengths[pos] = i->dataSize;
            m_paramFormats[pos] = 1;
        } break;
        case Memory::Abstract::Var::TYPE_VARCHAR:
        {
            m_paramValues[pos] = ABSTRACT_SPTR_AS(VARCHAR,m_inputVars[key])->getValue();
            m_paramLengths[pos] = strnlen(ABSTRACT_SPTR_AS(VARCHAR,m_inputVars[key])->getValue(),ABSTRACT_SPTR_AS(VARCHAR,m_inputVars[key])->getVarSize());
        } break;
        case Memory::Abstract::Var::TYPE_PTR:
        {
            void * ptr = ABSTRACT_SPTR_AS(PTR,m_inputVars[key])->getValue();
            // Threat PTR as char * (be careful, we should receive strlen compatible string, without null termination will result in an undefined behaviour)
            m_paramLengths[pos] = strnlen((char *)ptr,(0xFFFFFFFF/2)-1);
            m_paramValues[pos] = (char *) ptr;
        } break;
        case Memory::Abstract::Var::TYPE_STRING:
        {
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(STRING,m_inputVars[key])->toString());
        } break;
        case Memory::Abstract::Var::TYPE_STRINGLIST:
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(STRINGLIST,m_inputVars[key])->toString());
            break;
        case Memory::Abstract::Var::TYPE_IPV4:
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(IPV4,m_inputVars[key])->toString());
            break;
        case Memory::Abstract::Var::TYPE_MACADDR:
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(MACADDR,m_inputVars[key])->toString());
            break;
        case Memory::Abstract::Var::TYPE_IPV6:
            str = createDestroyableStringForInput(ABSTRACT_SPTR_AS(IPV6,m_inputVars[key])->toString());
            break;
        case Memory::Abstract::Var::TYPE_NULL:
            m_paramValues[pos] = nullptr;
            m_paramLengths[pos] = 0;
            break;
        }

        if (str)
        {
            m_paramValues[pos] = (char *)str->c_str();
            m_paramLengths[pos] = str->size();
        }
    }
    return true;
}

bool Query_PostgreSQL::exec0(const ExecType &execType, bool recursion)
{
    if (m_results)
    {
        throw std::runtime_error("Re-using queries is not supported.");
        return false;
    }

    // Prepare the query (will lock the db while using ppDb):
    ((SQLConnector_PostgreSQL*)m_pSQLConnector)->getDatabaseConnector(this);

    if (!m_databaseConnectionHandler)
        return false;

    // Prepare the query:
    m_results = PQexecPrepared(m_databaseConnectionHandler,
                            m_query.c_str(),
                            m_paramCount,
                            m_paramValues,
                            m_paramLengths,
                            m_paramFormats,
                            0);


    // Maybe is not connected or something failed very hard here.
    if (!m_results)
    {
        // While not connected...
        while (PQstatus(m_databaseConnectionHandler) != CONNECTION_OK && !recursion)
        {
            // Reconnect...
            if ( ((SQLConnector_PostgreSQL*)m_pSQLConnector)->reconnect(0xFFFFABCD) )
            {
                // If reconnected... execute the query again
                bool result2 = exec0(execType,true);

                // If there is any result, return the result...
                if (m_results)
                    return result2;
                // ...
                if (result2 == true)
                    throw std::runtime_error("how this can be true?.");

                // If no result (eg. disconnected again), then, keep the loop reconnecting
            }
            else
            {
                // The connection reached the max limit
                m_lastSQLError = "reconnection failed.";
                return false;
            }
        }
        m_lastSQLError = "connection failed.";
        return false;
    }

    m_execStatus = PQresultStatus(m_results);

    m_numRows=0;
    m_affectedRows=0;

    if (    m_execStatus==PGRES_BAD_RESPONSE
            ||
            m_execStatus==PGRES_FATAL_ERROR
            )
    {
        PQclear(m_results);
        m_results = nullptr;
        return false;
    }

    if (execType==EXEC_TYPE_SELECT)
    {
        m_numRows = PQntuples(m_results);
        return m_execStatus == PGRES_TUPLES_OK;
    }
    else
    {
        m_affectedRows = strtoull(PQcmdTuples(m_results),0,10);
        if (m_fetchLastInsertRowID)
             m_lastInsertRowID = PQoidValue(m_results);
        return m_execStatus == PGRES_COMMAND_OK;
    }
}
