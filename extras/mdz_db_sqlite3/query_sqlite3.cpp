#include "query_sqlite3.h"
#include "sqlconnector_sqlite3.h"
#include <mdz_mem_vars/a_allvars.h>
#include <string.h>

using namespace Mantids::Database;

Query_SQLite3::Query_SQLite3()
{
    stmt = nullptr;
    ppDb = nullptr;
    lastSQLReturnValue = SQLITE_OK;
}

Query_SQLite3::~Query_SQLite3()
{
    if (stmt)
    {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_finalize(stmt);
    }
}

bool Query_SQLite3::exec(const ExecType &execType)
{
    if (stmt)
    {
        throw std::runtime_error("Re-using queries is not supported.");
        return false;
    }

    // Prepare the query (will lock the db while using ppDb):
    ((SQLConnector_SQLite3*)sqlConnector)->putDatabaseConnectorIntoQuery(this);

    if (!ppDb)
        return false;

    std::unique_lock<std::mutex> lock(*mtDatabaseLock);

    const char *tail;
    // TODO: querylenght isn't -1?
    lastSQLReturnValue = sqlite3_prepare_v2(ppDb, query.c_str(), query.length(), &stmt, &tail);
    if ( lastSQLReturnValue != SQLITE_OK)
    {
        lastSQLError = "Error preparing the SQL query";
        return false;
    }

    // Bind the parameters (in and out)
    for ( const auto &inputVar : InputVars)
    {
        int idx = sqlite3_bind_parameter_index(stmt, inputVar.first.c_str());
        if (idx)
        {
            switch (inputVar.second->getVarType())
            {
            case Memory::Abstract::TYPE_BOOL:
                sqlite3_bind_int(stmt, idx, ABSTRACT_PTR_AS(BOOL,inputVar.second)->getValue()?1:0 );
                break;
            case Memory::Abstract::TYPE_INT8:
                sqlite3_bind_int(stmt, idx, ABSTRACT_PTR_AS(INT8,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::TYPE_INT16:
                sqlite3_bind_int(stmt, idx, ABSTRACT_PTR_AS(INT16,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::TYPE_INT32:
                sqlite3_bind_int(stmt, idx, ABSTRACT_PTR_AS(INT32,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::TYPE_INT64:
                sqlite3_bind_int64(stmt, idx, ABSTRACT_PTR_AS(INT64,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::TYPE_UINT8:
                sqlite3_bind_int(stmt, idx, ABSTRACT_PTR_AS(UINT8,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::TYPE_UINT16:
                sqlite3_bind_int(stmt, idx, ABSTRACT_PTR_AS(UINT16,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::TYPE_UINT32:
                sqlite3_bind_int64(stmt, idx, ABSTRACT_PTR_AS(UINT32,inputVar.second)->getValue() );
                break;
            case Memory::Abstract::TYPE_UINT64:
                // Not implemented.
                throw std::runtime_error("UINT64 is not supported by SQLite3 and can lead to precision errors, check your implementation");
                break;
            case Memory::Abstract::TYPE_DOUBLE:
                sqlite3_bind_double(stmt,idx,ABSTRACT_PTR_AS(DOUBLE,inputVar.second)->getValue());
                break;
            case Memory::Abstract::TYPE_BIN:
            {
                Memory::Abstract::sBinContainer * i = ABSTRACT_PTR_AS(BINARY,inputVar.second)->getValue();
#if SQLITE_VERSION_NUMBER>=3008007L
                sqlite3_bind_blob64(stmt,idx,i->ptr,i->dataSize,SQLITE_STATIC);
#else
                // Only support 2GB data on older sqlite3 versions... (http://www.sqlite.org/releaselog/3_8_7.html) - WARNING: the compatibility should be enforced in source side, not using containers with >2gb capacity...
                sqlite3_bind_blob(stmt,idx,i->ptr,i->dataSize,SQLITE_STATIC);
#endif
            } break;
            case Memory::Abstract::TYPE_VARCHAR:
            {
#if SQLITE_VERSION_NUMBER>=3008007L
                sqlite3_bind_text64(stmt,idx,ABSTRACT_PTR_AS(VARCHAR,inputVar.second)->getValue(),
                                    ABSTRACT_PTR_AS(VARCHAR,inputVar.second)->getVarSize(),
                                    SQLITE_STATIC,
                                    SQLITE_UTF8);
#else
                // Only support 2GB data on older sqlite3 versions... (http://www.sqlite.org/releaselog/3_8_7.html) - WARNING: the compatibility should be enforced in source side, not using containers with >2gb capacity...
                // Also the encoding is not defined...
                sqlite3_bind_text(stmt,idx,ABSTRACT_PTR_AS(VARCHAR,inputVar.second)->getValue(),
                                    ABSTRACT_PTR_AS(VARCHAR,inputVar.second)->getVarSize(),
                                    SQLITE_STATIC);
#endif
            } break;
            case Memory::Abstract::TYPE_DATETIME:
            {
                auto i = ABSTRACT_PTR_AS(DATETIME,inputVar.second)->toString();
                sqlite3_bind_text(stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::TYPE_STRING:
            {
                auto i = ABSTRACT_PTR_AS(STRING,inputVar.second)->toString();
                sqlite3_bind_text(stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::TYPE_STRINGLIST:
            {
                auto i = ABSTRACT_PTR_AS(STRINGLIST,inputVar.second)->toString();
                sqlite3_bind_text(stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::TYPE_IPV4:
            {
                auto i = ABSTRACT_PTR_AS(IPV4,inputVar.second)->toString();
                sqlite3_bind_text(stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::TYPE_MACADDR:
            {
                auto i = ABSTRACT_PTR_AS(MACADDR,inputVar.second)->toString();
                sqlite3_bind_text(stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::TYPE_IPV6:
            {
                auto i = ABSTRACT_PTR_AS(IPV6,inputVar.second)->toString();
                sqlite3_bind_text(stmt,idx,i.c_str(),i.size(),SQLITE_TRANSIENT);
            }break;
            case Memory::Abstract::TYPE_PTR:
            {
                void * ptr = ABSTRACT_PTR_AS(PTR,inputVar.second)->getValue();
                // Threat PTR as char * (be careful, we should receive strlen compatible string, without null termination will result in an undefined behaviour)
                size_t ptrSize = strnlen((char *)ptr,(0xFFFFFFFF/2)-1);
                sqlite3_bind_text(stmt,idx,(char *)ptr,ptrSize,SQLITE_STATIC);
            } break;
            case Memory::Abstract::TYPE_NULL:
                sqlite3_bind_null(stmt,idx);
                break;
            }
/*            lastSQLError = "Binding parameter for index not found";
            return false;*/
        }
    }

    numRows=0;
    affectedRows=0;

    // if insert, only do one step.
    if (execType == EXEC_TYPE_INSERT)
    {
        // execute...
        lastSQLReturnValue = sqlite3_step(stmt);

        // Get number of changes:
        affectedRows = sqlite3_changes(ppDb);

        if (bFetchLastInsertRowID)
             lastInsertRowID = sqlite3_last_insert_rowid(ppDb);

        if (!sqlite3IsDone())
        {
            lastSQLError = sqlite3_errmsg(ppDb);
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
    lastSQLReturnValue = sqlite3_step(stmt);

    if ( lastSQLReturnValue == SQLITE_ROW )
    {
        int columnpos = 0;
        for ( const auto &outputVar : resultVars)
        {
            isNull.push_back( sqlite3_column_type(stmt,columnpos) == SQLITE_NULL );

            switch (outputVar->getVarType())
            {
            case Memory::Abstract::TYPE_BOOL:
                ABSTRACT_PTR_AS(BOOL,outputVar)->setValue( sqlite3_column_int(stmt, columnpos)?true:false );
                break;
            case Memory::Abstract::TYPE_INT8:
                ABSTRACT_PTR_AS(INT8,outputVar)->setValue( sqlite3_column_int(stmt, columnpos) );
                break;
            case Memory::Abstract::TYPE_INT16:
                ABSTRACT_PTR_AS(INT16,outputVar)->setValue( sqlite3_column_int(stmt, columnpos) );
                break;
            case Memory::Abstract::TYPE_INT32:
                ABSTRACT_PTR_AS(INT32,outputVar)->setValue( sqlite3_column_int(stmt, columnpos) );
                break;
            case Memory::Abstract::TYPE_INT64:
                ABSTRACT_PTR_AS(INT64,outputVar)->setValue( sqlite3_column_int64(stmt, columnpos) );
                break;
            case Memory::Abstract::TYPE_UINT8:
                ABSTRACT_PTR_AS(UINT8,outputVar)->setValue( sqlite3_column_int(stmt, columnpos) );
                break;
            case Memory::Abstract::TYPE_UINT16:
                ABSTRACT_PTR_AS(UINT16,outputVar)->setValue( sqlite3_column_int(stmt, columnpos) );
                break;
            case Memory::Abstract::TYPE_UINT32:
                ABSTRACT_PTR_AS(UINT32,outputVar)->setValue( sqlite3_column_int64(stmt, columnpos) );
                break;
            case Memory::Abstract::TYPE_UINT64:
                // Not implemented.
                throw std::runtime_error("UINT64 is not supported by SQLite3 and can lead to precision errors, check your implementation");
                break;
            case Memory::Abstract::TYPE_DOUBLE:
                ABSTRACT_PTR_AS(DOUBLE,outputVar)->setValue( sqlite3_column_double(stmt, columnpos) );
                break;
            case Memory::Abstract::TYPE_BIN:
            {
                Memory::Abstract::sBinContainer binContainer;
                binContainer.ptr = (char *)sqlite3_column_blob(stmt,columnpos);
                // TODO: should bytes need to be 64-bit for blob64?
                binContainer.dataSize = sqlite3_column_bytes(stmt,columnpos);
                ABSTRACT_PTR_AS(BINARY,outputVar)->setValue( &binContainer );
                binContainer.ptr = nullptr; // don't destroy the data.
            } break;
            case Memory::Abstract::TYPE_VARCHAR:
            {
                // This will copy the memory.
                ABSTRACT_PTR_AS(VARCHAR,outputVar)->setValue( (char *)sqlite3_column_text(stmt,columnpos) );
            } break;
            case Memory::Abstract::TYPE_STRING:
            {
                ABSTRACT_PTR_AS(STRING,outputVar)->setValue( (char *)sqlite3_column_text(stmt,columnpos) );
            }break;
            case Memory::Abstract::TYPE_STRINGLIST:
            {
                ABSTRACT_PTR_AS(STRINGLIST,outputVar)->fromString( (char *)sqlite3_column_text(stmt,columnpos) );
            }break;
            case Memory::Abstract::TYPE_DATETIME:
            {
                ABSTRACT_PTR_AS(DATETIME,outputVar)->fromString( (char *)sqlite3_column_text(stmt,columnpos) );
            }break;
            case Memory::Abstract::TYPE_IPV4:
            {
                ABSTRACT_PTR_AS(IPV4,outputVar)->fromString( (char *)sqlite3_column_text(stmt,columnpos) );
            }break;
            case Memory::Abstract::TYPE_MACADDR:
            {
                ABSTRACT_PTR_AS(MACADDR,outputVar)->fromString( (char *)sqlite3_column_text(stmt,columnpos) );
            }break;
            case Memory::Abstract::TYPE_IPV6:
            {
                ABSTRACT_PTR_AS(IPV6,outputVar)->fromString( (char *)sqlite3_column_text(stmt,columnpos) );
            }break;
            case Memory::Abstract::TYPE_PTR:
            {
                // This will reference the memory, but will disappear on the next step
                ABSTRACT_PTR_AS(PTR,outputVar)->setValue( (char *)sqlite3_column_text(stmt,columnpos) );
            } break;
            case Memory::Abstract::TYPE_NULL:
                // Don't copy the value (not needed).
                break;
            }
            columnpos++;
        }
    }

    return lastSQLReturnValue == SQLITE_ROW;
}

bool Query_SQLite3::sqlite3IsDone() const
{
    return lastSQLReturnValue == SQLITE_DONE;
}

void Query_SQLite3::sqlite3SetDatabaseConnector(sqlite3 *ppDb)
{
    this->ppDb = ppDb;
}

