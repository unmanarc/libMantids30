#include "query_sqlite3.h"
#include "sqlconnector_sqlite3.h"
#include <Mantids30/Memory/a_allvars.h>
#include <string.h>

#include <stdexcept>

using namespace Mantids30::Database;

Query_SQLite3::Query_SQLite3()
{
    m_stmt = nullptr;
    m_databaseConnectionHandler = nullptr;
    m_lastSQLReturnValue = SQLITE_OK;
}

Query_SQLite3::~Query_SQLite3()
{
    if (m_stmt)
    {
        sqlite3_reset(m_stmt);
        sqlite3_clear_bindings(m_stmt);
        sqlite3_finalize(m_stmt);
    }
}

bool Query_SQLite3::exec0(const ExecType &execType, bool recursion)
{
    if (m_stmt)
    {
        throw std::runtime_error("Re-using queries is not supported.");
        return false;
    }

    // Prepare the query (will lock the db while using ppDb):
    ((SQLConnector_SQLite3*)m_pSQLConnector)->putDatabaseConnectorIntoQuery(this);

    if (!m_databaseConnectionHandler)
        return false;

    const char *tail;
    // TODO: querylenght isn't -1?
    m_lastSQLReturnValue = sqlite3_prepare_v2(m_databaseConnectionHandler, m_query.c_str(), m_query.length(), &m_stmt, &tail);
    if ( m_lastSQLReturnValue != SQLITE_OK)
    {
        m_lastSQLError = std::string(sqlite3_errmsg(m_databaseConnectionHandler));
        if (m_throwCPPErrorOnQueryFailure)
        {
            throw std::runtime_error("Error preparing the query: " + m_lastSQLError);
        }
        return false;
    }

    // Bind the parameters (in and out)
    for ( const auto &inputVar : m_inputVars)
    {
        int idx = sqlite3_bind_parameter_index(m_stmt, inputVar.first.c_str());
        if (idx)
        {
            switch (inputVar.second->getVarType())
            {
            case Memory::Abstract::Var::TYPE_BOOL:
                sqlite3_bind_int(m_stmt, idx, ABSTRACT_SPTR_AS(BOOL,inputVar.second)->getValue()?1:0 );
                break;
            case Memory::Abstract::Var::TYPE_INT8:
                sqlite3_bind_int(m_stmt, idx, ABSTRACT_SPTR_AS(INT8,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::Var::TYPE_INT16:
                sqlite3_bind_int(m_stmt, idx, ABSTRACT_SPTR_AS(INT16,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::Var::TYPE_INT32:
                sqlite3_bind_int(m_stmt, idx, ABSTRACT_SPTR_AS(INT32,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::Var::TYPE_INT64:
                sqlite3_bind_int64(m_stmt, idx, ABSTRACT_SPTR_AS(INT64,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::Var::TYPE_UINT8:
                sqlite3_bind_int(m_stmt, idx, ABSTRACT_SPTR_AS(UINT8,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::Var::TYPE_UINT16:
                sqlite3_bind_int(m_stmt, idx, ABSTRACT_SPTR_AS(UINT16,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::Var::TYPE_UINT32:
                sqlite3_bind_int64(m_stmt, idx, ABSTRACT_SPTR_AS(UINT32,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::Var::TYPE_UINT64:
                // Not implemented.
                throw std::runtime_error("UINT64 is not supported by SQLite3 and can lead to precision errors, check your implementation");
                break;
            case Memory::Abstract::Var::TYPE_DOUBLE:
                sqlite3_bind_double(m_stmt,idx,ABSTRACT_SPTR_AS(DOUBLE,inputVar.second)->getValue());
                break;
            case Memory::Abstract::Var::TYPE_BIN:
            {
                Memory::Abstract::BINARY::sBinContainer * i = ABSTRACT_SPTR_AS(BINARY,inputVar.second)->getValue();
#if SQLITE_VERSION_NUMBER>=3008007L
                sqlite3_bind_blob64(m_stmt,idx,i->ptr,i->dataSize,SQLITE_STATIC);
#else
                // Only support 2GB data on older sqlite3 versions... (http://www.sqlite.org/releaselog/3_8_7.html) - WARNING: the compatibility should be enforced in source side, not using containers with >2gb capacity...
                sqlite3_bind_blob(stmt,idx,i->ptr,i->dataSize,SQLITE_STATIC);
#endif
            } break;
            case Memory::Abstract::Var::TYPE_VARCHAR:
            {
#if SQLITE_VERSION_NUMBER>=3008007L
                sqlite3_bind_text64(m_stmt,idx,ABSTRACT_SPTR_AS(VARCHAR,inputVar.second)->getValue(),
                                    ABSTRACT_SPTR_AS(VARCHAR,inputVar.second)->getVarSize(),
                                    SQLITE_STATIC,
                                    SQLITE_UTF8);
#else
                // Only support 2GB data on older sqlite3 versions... (http://www.sqlite.org/releaselog/3_8_7.html) - WARNING: the compatibility should be enforced in source side, not using containers with >2gb capacity...
                // Also the encoding is not defined...
                sqlite3_bind_text(stmt,idx,ABSTRACT_SPTR_AS(VARCHAR,inputVar.second)->getValue(),
                                    ABSTRACT_SPTR_AS(VARCHAR,inputVar.second)->getVarSize(),
                                    SQLITE_STATIC);
#endif
            } break;
            case Memory::Abstract::Var::TYPE_DATETIME:
            {
                auto i = ABSTRACT_SPTR_AS(DATETIME,inputVar.second)->toString();
                sqlite3_bind_text(m_stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::Var::TYPE_STRING:
            {
                auto i = ABSTRACT_SPTR_AS(STRING,inputVar.second)->toString();
                sqlite3_bind_text(m_stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::Var::TYPE_STRINGLIST:
            {
                auto i = ABSTRACT_SPTR_AS(STRINGLIST,inputVar.second)->toString();
                sqlite3_bind_text(m_stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::Var::TYPE_IPV4:
            {
                auto i = ABSTRACT_SPTR_AS(IPV4,inputVar.second)->toString();
                sqlite3_bind_text(m_stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::Var::TYPE_MACADDR:
            {
                auto i = ABSTRACT_SPTR_AS(MACADDR,inputVar.second)->toString();
                sqlite3_bind_text(m_stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::Var::TYPE_IPV6:
            {
                auto i = ABSTRACT_SPTR_AS(IPV6,inputVar.second)->toString();
                sqlite3_bind_text(m_stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::Var::TYPE_PTR:
            {
                void * ptr = ABSTRACT_SPTR_AS(PTR,inputVar.second)->getValue();
                // Threat PTR as char * (be careful, we should receive strlen compatible string, without null termination will result in an undefined behaviour)
                size_t ptrSize = strnlen((char *)ptr,(0xFFFFFFFF/2)-1);
                sqlite3_bind_text(m_stmt,idx,(char *)ptr,ptrSize,SQLITE_STATIC);
            } break;
            case Memory::Abstract::Var::TYPE_NULL:
                sqlite3_bind_null(m_stmt,idx);
                break;
            }
/*            lastSQLError = "Binding parameter for index not found";
            return false;*/
        }
    }

    m_numRows=0;
    m_affectedRows=0;

    // if insert, only do one step.
    if (execType == EXEC_TYPE_INSERT)
    {
        // execute...
        m_lastSQLReturnValue = sqlite3_step(m_stmt);

        // Get number of changes:
        m_affectedRows = sqlite3_changes(m_databaseConnectionHandler);

        if (m_fetchLastInsertRowID)
             m_lastInsertRowID = sqlite3_last_insert_rowid(m_databaseConnectionHandler);

        if (!sqlite3IsDone())
        {
            m_lastSQLError = sqlite3_errmsg(m_databaseConnectionHandler);
            if (m_throwCPPErrorOnQueryFailure)
            {
                throw std::runtime_error("Error during insert query: " + m_lastSQLError);
            }
            return false;
        }
        else
            return true;
    }
    else
    {
        // Execution should be done on step0.
        // TODO: retrieve number of retrieved rows in numRows.
    }

    return true;
}

bool Query_SQLite3::step0()
{
    m_lastSQLReturnValue = sqlite3_step(m_stmt);

    if ( m_lastSQLReturnValue == SQLITE_ROW )
    {
        int columnpos = 0;
        for ( const auto &outputVar : m_resultVars)
        {
            m_fieldIsNull.push_back( sqlite3_column_type(m_stmt,columnpos) == SQLITE_NULL );

            switch (outputVar->getVarType())
            {
            case Memory::Abstract::Var::TYPE_BOOL:
                ABSTRACT_PTR_AS(BOOL,outputVar)->setValue( sqlite3_column_int(m_stmt, columnpos)?true:false );
                break;
            case Memory::Abstract::Var::TYPE_INT8:
                ABSTRACT_PTR_AS(INT8,outputVar)->setValue( sqlite3_column_int(m_stmt, columnpos) );
                break;
            case Memory::Abstract::Var::TYPE_INT16:
                ABSTRACT_PTR_AS(INT16,outputVar)->setValue( sqlite3_column_int(m_stmt, columnpos) );
                break;
            case Memory::Abstract::Var::TYPE_INT32:
                ABSTRACT_PTR_AS(INT32,outputVar)->setValue( sqlite3_column_int(m_stmt, columnpos) );
                break;
            case Memory::Abstract::Var::TYPE_INT64:
                ABSTRACT_PTR_AS(INT64,outputVar)->setValue( sqlite3_column_int64(m_stmt, columnpos) );
                break;
            case Memory::Abstract::Var::TYPE_UINT8:
                ABSTRACT_PTR_AS(UINT8,outputVar)->setValue( sqlite3_column_int(m_stmt, columnpos) );
                break;
            case Memory::Abstract::Var::TYPE_UINT16:
                ABSTRACT_PTR_AS(UINT16,outputVar)->setValue( sqlite3_column_int(m_stmt, columnpos) );
                break;
            case Memory::Abstract::Var::TYPE_UINT32:
                ABSTRACT_PTR_AS(UINT32,outputVar)->setValue( sqlite3_column_int64(m_stmt, columnpos) );
                break;
            case Memory::Abstract::Var::TYPE_UINT64:
                // Not implemented.
                throw std::runtime_error("UINT64 is not supported by SQLite3 and can lead to precision errors, check your implementation");
                break;
            case Memory::Abstract::Var::TYPE_DOUBLE:
                ABSTRACT_PTR_AS(DOUBLE,outputVar)->setValue( sqlite3_column_double(m_stmt, columnpos) );
                break;
            case Memory::Abstract::Var::TYPE_BIN:
            {
                Memory::Abstract::BINARY::sBinContainer binContainer;
                binContainer.ptr = (char *)sqlite3_column_blob(m_stmt,columnpos);
                // TODO: should bytes need to be 64-bit for blob64?
                binContainer.dataSize = sqlite3_column_bytes(m_stmt,columnpos);
                ABSTRACT_PTR_AS(BINARY,outputVar)->setValue( &binContainer );
                binContainer.ptr = nullptr; // don't destroy the data.
            } break;
            case Memory::Abstract::Var::TYPE_VARCHAR:
            {
                // This will copy the memory.
                ABSTRACT_PTR_AS(VARCHAR,outputVar)->setValue( (char *)sqlite3_column_text(m_stmt,columnpos) );
            } break;
            case Memory::Abstract::Var::TYPE_STRING:
            {
                ABSTRACT_PTR_AS(STRING,outputVar)->setValue( (char *)sqlite3_column_text(m_stmt,columnpos) );
            }break;
            case Memory::Abstract::Var::TYPE_STRINGLIST:
            {
                ABSTRACT_PTR_AS(STRINGLIST,outputVar)->fromString( (char *)sqlite3_column_text(m_stmt,columnpos) );
            }break;
            case Memory::Abstract::Var::TYPE_DATETIME:
            {
                ABSTRACT_PTR_AS(DATETIME,outputVar)->fromString( (char *)sqlite3_column_text(m_stmt,columnpos) );
            }break;
            case Memory::Abstract::Var::TYPE_IPV4:
            {
                ABSTRACT_PTR_AS(IPV4,outputVar)->fromString( (char *)sqlite3_column_text(m_stmt,columnpos) );
            }break;
            case Memory::Abstract::Var::TYPE_MACADDR:
            {
                ABSTRACT_PTR_AS(MACADDR,outputVar)->fromString( (char *)sqlite3_column_text(m_stmt,columnpos) );
            }break;
            case Memory::Abstract::Var::TYPE_IPV6:
            {
                ABSTRACT_PTR_AS(IPV6,outputVar)->fromString( (char *)sqlite3_column_text(m_stmt,columnpos) );
            }break;
            case Memory::Abstract::Var::TYPE_PTR:
            {
                // This will reference the memory, but will disappear on the next step
                ABSTRACT_PTR_AS(PTR,outputVar)->setValue( (char *)sqlite3_column_text(m_stmt,columnpos) );
            } break;
            case Memory::Abstract::Var::TYPE_NULL:
                // Don't copy the value (not needed).
                break;
            }
            columnpos++;
        }
    }

    return m_lastSQLReturnValue == SQLITE_ROW;
}

void Query_SQLite3::setDatabaseConnectionHandler(sqlite3 *newPpDb)
{
    m_databaseConnectionHandler = newPpDb;
}

bool Query_SQLite3::sqlite3IsDone() const
{
    return m_lastSQLReturnValue == SQLITE_DONE;
}

