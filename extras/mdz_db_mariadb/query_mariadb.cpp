#include "query_mariadb.h"
#include "sqlconnector_mariadb.h"
#include <string.h>
#include <mdz_mem_vars/a_allvars.h>
#include <mdz_hlp_functions/mem.h>

#include <errmsg.h>


#include <stdexcept>

// From: https://bugs.mysql.com/?id=87337
#if !defined(MARIADB_BASE_VERSION) && \
    !defined(MARIADB_VERSION_ID) && \
    MYSQL_VERSION_ID >= 80001 && \
    MYSQL_VERSION_ID != 80002
typedef bool my_bool;
#endif

using namespace Mantids::Database;

Query_MariaDB::Query_MariaDB()
{
    stmt = nullptr;
    bindedInputParams = nullptr;
    bindedResultsParams = nullptr;

    dbCnt = nullptr;
}

Query_MariaDB::~Query_MariaDB()
{
    // Destroy binded values.
    for (size_t pos=0;pos<keysByPos.size();pos++)
    {
        if (bindedInputParams[pos].buffer)
        {
            if (bindedInputParams[pos].buffer_type == MYSQL_TYPE_LONGLONG && bindedInputParams[pos].is_unsigned)
            {
                if (bindedInputParams[pos].is_unsigned)
                {
                    unsigned long long * buffer = (unsigned long long *)bindedInputParams[pos].buffer;
                    delete buffer;
                }
                else
                {
                    long long * buffer = (long long *)bindedInputParams[pos].buffer;
                    delete buffer;
                }
            }
            else if (bindedInputParams[pos].buffer_type == MYSQL_TYPE_DOUBLE)
            {
                double * buffer = (double *)bindedInputParams[pos].buffer;
                delete buffer;
            }
            else if (bindedInputParams[pos].buffer_type == MYSQL_TYPE_LONG)
            {
                if (bindedInputParams[pos].is_unsigned)
                {
                    unsigned long * buffer = (unsigned long *)bindedInputParams[pos].buffer;
                    delete buffer;
                }
                else
                {
                    long * buffer = (long *)bindedInputParams[pos].buffer;
                    delete buffer;
                }
            }
            else if (bindedInputParams[pos].buffer_type == MYSQL_TYPE_TINY)
            {
                if (bindedInputParams[pos].is_unsigned)
                {
                    unsigned char * buffer = (unsigned char *)bindedInputParams[pos].buffer;
                    delete buffer;
                }
                else
                {
                    char * buffer = (char *)bindedInputParams[pos].buffer;
                    delete buffer;
                }
            }
            else if (bindedInputParams[pos].buffer_type == MYSQL_TYPE_SHORT)
            {
                if (bindedInputParams[pos].is_unsigned)
                {
                    unsigned short * buffer = (unsigned short *)bindedInputParams[pos].buffer;
                    delete buffer;
                }
                else
                {
                    short * buffer = (short *)bindedInputParams[pos].buffer;
                    delete buffer;
                }
            }

        }
    }

    // Destroy main items:
    if (bindedInputParams) delete [] bindedInputParams;
    if (bindedResultsParams) delete [] bindedResultsParams;

    // Destroy the statement
    if (stmt)
    {
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
        stmt = NULL;
    }
}
bool Query_MariaDB::step0()
{
    bool r = mysql_stmt_fetch (stmt) == 0;

    if (!r)
        return false;

    // Now bind each variable.
    for ( size_t col=0; col<resultVars.size(); col++ )
    {
        Memory::Abstract::Var * val = resultVars[col];

        Memory::Abstract::BINARY::sBinContainer * sBin = nullptr;

        unsigned char cRetBoolean=0;
        MYSQL_BIND result = {};
        my_bool bIsNull;

        ZeroBStruct(result);
        result.is_null = &bIsNull;

        switch (resultVars[col]->getVarType())
        {
        case Memory::Abstract::Var::TYPE_BOOL:
            result.buffer_type = MYSQL_TYPE_TINY;
            result.is_unsigned = 0;
            result.buffer = &(cRetBoolean);
            break;
        case Memory::Abstract::Var::TYPE_INT8:
            result.buffer_type = MYSQL_TYPE_TINY;
            result.is_unsigned = 0;
            result.buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::Var::TYPE_INT16:
            result.buffer_type = MYSQL_TYPE_SHORT;
            result.is_unsigned = 0;
            result.buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::Var::TYPE_INT32:
            result.buffer_type = MYSQL_TYPE_LONG;
            result.buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::Var::TYPE_INT64:
            result.buffer_type = MYSQL_TYPE_LONGLONG;
            result.buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::Var::TYPE_UINT8:
            result.buffer_type = MYSQL_TYPE_TINY;
            result.is_unsigned = 1;
            result.buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::Var::TYPE_UINT16:
            result.buffer_type = MYSQL_TYPE_SHORT;
            result.is_unsigned = 1;
            result.buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::Var::TYPE_UINT32:
            result.buffer_type = MYSQL_TYPE_LONG;
            result.is_unsigned = 1;
            result.buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::Var::TYPE_UINT64:
            result.buffer_type = MYSQL_TYPE_LONGLONG;
            result.is_unsigned = 1;
            result.buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::Var::TYPE_DOUBLE:
            result.buffer_type = MYSQL_TYPE_DOUBLE;
            result.is_unsigned = 0;
            result.buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::Var::TYPE_BIN:
        {
            unsigned long varSize = mariaDBfetchVarSize(col,MYSQL_TYPE_BLOB);
            if (varSize>0)
            {
                sBin = new Memory::Abstract::BINARY::sBinContainer(varSize);
                result.buffer_type = MYSQL_TYPE_BLOB;
                result.buffer = sBin->ptr;
                result.buffer_length = sBin->dataSize;
                // TODO: handle truncated?
            }
            // fetch later.
        } break;
        case Memory::Abstract::Var::TYPE_VARCHAR:
        {
            // This will copy the memory.
            result.buffer_type = MYSQL_TYPE_STRING;
            result.length = ABSTRACT_PTR_AS(VARCHAR,val)->getFillSizePTR();
            result.buffer = val->getDirectMemory();
            result.buffer_length = ABSTRACT_PTR_AS(VARCHAR,val)->getVarSize();
        } break;
        case Memory::Abstract::Var::TYPE_STRING:
        case Memory::Abstract::Var::TYPE_STRINGLIST:
        case Memory::Abstract::Var::TYPE_DATETIME:
        case Memory::Abstract::Var::TYPE_IPV4:
        case Memory::Abstract::Var::TYPE_MACADDR:
        case Memory::Abstract::Var::TYPE_IPV6:
        case Memory::Abstract::Var::TYPE_PTR:
        {
            unsigned long varSize = mariaDBfetchVarSize(col,MYSQL_TYPE_STRING);
            if (varSize>0)
            {
                sBin = new Memory::Abstract::BINARY::sBinContainer(varSize);
                result.buffer_type = MYSQL_TYPE_STRING;
                result.buffer = sBin->ptr;
                result.buffer_length = sBin->dataSize;
                // TODO: handle truncated?
            }
        } break;
        case Memory::Abstract::Var::TYPE_NULL:
            // Don't copy the value (not needed).
            break;
        }

        // fetch the column:
        mysql_stmt_fetch_column(stmt, &result, col, 0);

        if ( resultVars[col]->getVarType() == Memory::Abstract::Var::TYPE_BOOL )
        {
            ABSTRACT_PTR_AS(BOOL,val)->setValue(cRetBoolean);
        }

        if (sBin)
        {
            switch (resultVars[col]->getVarType())
            {
            case Memory::Abstract::Var::TYPE_BIN:
            {
                ABSTRACT_PTR_AS(BINARY,resultVars[col])->setValue( sBin );
            }break;
            case Memory::Abstract::Var::TYPE_STRING:
            {
                ABSTRACT_PTR_AS(STRING,resultVars[col])->setValue( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_STRINGLIST:
            {
                ABSTRACT_PTR_AS(STRINGLIST,resultVars[col])->fromString( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_DATETIME:
            {
                ABSTRACT_PTR_AS(DATETIME,resultVars[col])->fromString( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_IPV4:
            {
                ABSTRACT_PTR_AS(IPV4,resultVars[col])->fromString( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_MACADDR:
            {
                ABSTRACT_PTR_AS(MACADDR,resultVars[col])->fromString( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_IPV6:
            {
                ABSTRACT_PTR_AS(IPV6,resultVars[col])->fromString( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_PTR:
            {
                ABSTRACT_PTR_AS(PTR,resultVars[col])->setValue((void *)createDestroyableStringForResults(sBin->ptr)->c_str());
            }break;
            default:
                break;
            }

            delete sBin;
        }

        isNull.push_back(bIsNull);
    }

    return true;
}

void Query_MariaDB::mariaDBSetDatabaseConnector(MYSQL *dbCnt)
{
    this->dbCnt = dbCnt;
}

bool Query_MariaDB::postBindInputVars()
{
    // Load Keys:
    std::list<std::string> keysIn;
    for (auto & i : InputVars) keysIn.push_back(i.first);

    // Replace the keys for ?:
    while (replaceFirstKey(query,keysIn,keysByPos, "?"))
    {}

    if (!keysByPos.size())
        return true;

    // Create the bind struct...
    bindedInputParams = new MYSQL_BIND[keysByPos.size()];

    for (size_t pos=0; pos<keysByPos.size(); pos++)
    {
        ZeroBStruct(bindedInputParams[pos]);

        bindedInputParams[pos].is_unsigned = 0;
        bindedInputParams[pos].length = 0;

        /*
        Bind params here.
        */
        switch (InputVars[ keysByPos[pos] ]->getVarType())
        {
        case Memory::Abstract::Var::TYPE_BOOL:
        {
            unsigned char * buffer = new unsigned char;
            buffer[0] = ABSTRACT_PTR_AS(BOOL,InputVars[ keysByPos[pos] ])->getValue()?1:0;

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_TINY;
            bindedInputParams[pos].buffer = (char *) buffer;
            bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::Var::TYPE_INT8:
        {
            char * buffer = new char;
            (*buffer) = ABSTRACT_PTR_AS(INT8,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_TINY;
            bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::Var::TYPE_INT16:
        {
            short * buffer = new short;
            (*buffer) = ABSTRACT_PTR_AS(INT16,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_SHORT;
            bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::Var::TYPE_INT32:
        {
            long * buffer = new long;
            (*buffer) = ABSTRACT_PTR_AS(INT32,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::Var::TYPE_INT64:
        {
            long long * buffer = new long long;
            (*buffer) = ABSTRACT_PTR_AS(INT64,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONGLONG;
            bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::Var::TYPE_UINT8:
        {
            unsigned char * buffer = new unsigned char;
            (*buffer) = ABSTRACT_PTR_AS(UINT8,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_TINY;
            bindedInputParams[pos].buffer = (char *) buffer;
            bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::Var::TYPE_UINT16:
        {
            unsigned short * buffer = new unsigned short;
            (*buffer) = ABSTRACT_PTR_AS(UINT16,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_SHORT;
            bindedInputParams[pos].buffer = (char *) buffer;
            bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::Var::TYPE_UINT32:
        {
            unsigned long * buffer = new unsigned long;
            (*buffer) = ABSTRACT_PTR_AS(UINT32,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedInputParams[pos].buffer = (char *) buffer;
            bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::Var::TYPE_UINT64:
        {
            unsigned long long * buffer = new unsigned long long;
            (*buffer) = ABSTRACT_PTR_AS(UINT64,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONGLONG;
            bindedInputParams[pos].buffer = (char *) buffer;
            bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::Var::TYPE_DOUBLE:
        {
            double * buffer = new double;
            (*buffer) = ABSTRACT_PTR_AS(DOUBLE,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_DOUBLE;
            bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::Var::TYPE_BIN:
        {
            Memory::Abstract::BINARY::sBinContainer * i = ABSTRACT_PTR_AS(BINARY,InputVars[ keysByPos[pos] ])->getValue();
            bindedInputParams[pos].buffer_type = MYSQL_TYPE_BLOB;
            bindedInputParams[pos].buffer_length = i->dataSize;
            bindedInputParams[pos].buffer = (char *) i->ptr;
        } break;
        case Memory::Abstract::Var::TYPE_VARCHAR:
        {
            bindedInputParams[pos].buffer_type = MYSQL_TYPE_STRING;
            bindedInputParams[pos].buffer_length = strnlen(ABSTRACT_PTR_AS(VARCHAR,InputVars[ keysByPos[pos] ])->getValue(),ABSTRACT_PTR_AS(VARCHAR,InputVars[ keysByPos[pos] ])->getVarSize())+1;
            bindedInputParams[pos].buffer = (char *) ABSTRACT_PTR_AS(VARCHAR,InputVars[ keysByPos[pos] ])->getValue();
        } break;
        case Memory::Abstract::Var::TYPE_PTR:
        {
            void * ptr = ABSTRACT_PTR_AS(PTR,InputVars[ keysByPos[pos] ])->getValue();
            // Threat PTR as char * (be careful, we should receive strlen compatible string, without null termination will result in an undefined behaviour)
            bindedInputParams[pos].buffer_type = MYSQL_TYPE_STRING;
            bindedInputParams[pos].buffer_length = strnlen((char *)ptr,0xFFFFFFFF);
            bindedInputParams[pos].buffer = (char *) ptr;
        } break;

        case Memory::Abstract::Var::TYPE_STRING:
        case Memory::Abstract::Var::TYPE_STRINGLIST:
        case Memory::Abstract::Var::TYPE_DATETIME:
        case Memory::Abstract::Var::TYPE_MACADDR:
        case Memory::Abstract::Var::TYPE_IPV4:
        case Memory::Abstract::Var::TYPE_IPV6:
        {
            std::string * str = nullptr;

            switch (InputVars[ keysByPos[pos] ]->getVarType())
            {
            case Memory::Abstract::Var::TYPE_STRING:
            {
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(STRING,InputVars[ keysByPos[pos] ])->getValue());
            } break;
            case Memory::Abstract::Var::TYPE_STRINGLIST:
            {
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(STRINGLIST,InputVars[ keysByPos[pos] ])->toString());
            } break;
            case Memory::Abstract::Var::TYPE_DATETIME:
            {
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(DATETIME,InputVars[ keysByPos[pos] ])->toString());
            } break;
            case Memory::Abstract::Var::TYPE_IPV4:
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(IPV4,InputVars[ keysByPos[pos] ])->toString());
                break;
            case Memory::Abstract::Var::TYPE_MACADDR:
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(MACADDR,InputVars[ keysByPos[pos] ])->toString());
                break;
            case Memory::Abstract::Var::TYPE_IPV6:
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(IPV6,InputVars[ keysByPos[pos] ])->toString());
                break;
            default:
                break;
            }

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_STRING;

            if (str)
            {
                bindedInputParams[pos].buffer_length = str->size();
                bindedInputParams[pos].buffer = (char *) str->c_str();
            }
            else
            {
                bindedInputParams[pos].buffer_length = 0;
                bindedInputParams[pos].buffer = 0;
            }
        } break;
        case Memory::Abstract::Var::TYPE_NULL:
            bindedInputParams[pos].is_null_value = 1;
            break;
        }
    }

    return true;
}

bool Query_MariaDB::connectionError()
{
    return (    lastSQLReturnValue == CR_CONN_HOST_ERROR
            ||  lastSQLReturnValue == CR_SERVER_GONE_ERROR
            ||  lastSQLReturnValue == CR_CONNECTION_ERROR
                ||  lastSQLReturnValue == CR_SERVER_LOST
                ||  lastSQLReturnValue ==CR_UNKNOWN_HOST
                );
}

bool Query_MariaDB::exec0(const ExecType &execType, bool recursion)
{
    lastSQLReturnValue = 0;

    if (stmt)
    {
        throw std::runtime_error("Re-using queries is not supported.");
        return false;
    }

    ((SQLConnector_MariaDB*)sqlConnector)->getDatabaseConnector(this);

    if (!dbCnt)
        return false;

    // Prepare the query (will lock the db while using ppDb):
    stmt = mysql_stmt_init(dbCnt);
    if (stmt==nullptr)
    {
        return false;
    }

    /////////////////
    // Prepare the statement
    if ((lastSQLReturnValue = mysql_stmt_prepare(stmt, query.c_str(), query.size())) != 0)
    {
        lastSQLError = mysql_stmt_error(stmt);
        return false;
    }

    ////////////////
    // Count/convert the parameters into "?"
    // Now we have an ordered array with the keys:
    if (mysql_stmt_bind_param(stmt, bindedInputParams))
    {
        lastSQLError = mysql_stmt_error(stmt);
        return false;
    }

    ///////////////
    // Execute!!
    if ((lastSQLReturnValue = mysql_stmt_execute(stmt)) != 0)
    {
        // Reconnection:
        while ( connectionError() && !recursion )
        {
            // Reconnect here until timeout...
            if (((SQLConnector_MariaDB*)sqlConnector)->reconnect(0xFFFFABCD))
            {
                // Remove the prepared statement...
                if (stmt)
                {
                    mysql_stmt_free_result(stmt);
                    mysql_stmt_close(stmt);
                    stmt = NULL;
                }

                // Reconnected... executing the query again...
                bool result2 = exec0(execType,true);

                // if Resulted in another error or success
                if (!connectionError())
                    return result2;

                // Otherwise, keep reconnecting...

                // ...
                if (result2 == true)
                    throw std::runtime_error("how this can be true?.");
            }
            else
            {
                // Not reconnected, timed out...
                lastSQLError = "reconnection failed.";
                return false;
            }
        }

        // When failed with another error:
        lastSQLError = mysql_stmt_error(stmt);
        return false;
    }

    numRows=0;
    affectedRows=0;

    if(mysql_stmt_store_result(stmt)!=0)
    {
        lastSQLError = mysql_stmt_error(stmt);
        return false;
    }

    ///////////////
    if (execType != EXEC_TYPE_SELECT)
    {
        if (bFetchLastInsertRowID)
            lastInsertRowID = mysql_stmt_insert_id(stmt);
        affectedRows = mysql_stmt_num_rows(stmt);
    }
    else
    {
        numRows = mysql_stmt_num_rows(stmt);
    }

    return true;
}


unsigned long Query_MariaDB::mariaDBfetchVarSize(const size_t &col, const enum_field_types & fieldType)
{
    unsigned long r=0;
//    static const size_t szBuffer = 64;

    my_bool isTruncated = 0;
    char aBuffer[64];
  //  std::array<char, szBuffer> aBuffer;

    MYSQL_BIND bind = {};
    bind.buffer_type = fieldType;
    bind.buffer = aBuffer;
    bind.buffer_length = 64;
    bind.is_null = 0;
    bind.length = &r;
    bind.error = &isTruncated;

    if (mysql_stmt_fetch_column(stmt, &bind, col, 0)==0)
        return r;
    else
        return 0;
}

