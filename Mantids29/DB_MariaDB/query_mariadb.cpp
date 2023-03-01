#include "query_mariadb.h"
#include "sqlconnector_mariadb.h"
#include <string.h>
#include <Mantids29/Memory/a_allvars.h>
#include <Mantids29/Helpers/mem.h>

#include <errmsg.h>


#include <stdexcept>

// From: https://bugs.mysql.com/?id=87337
#if !defined(MARIADB_BASE_VERSION) && \
    !defined(MARIADB_VERSION_ID) && \
    MYSQL_VERSION_ID >= 80001 && \
    MYSQL_VERSION_ID != 80002
typedef bool my_bool;
#endif

using namespace Mantids29::Database;

Query_MariaDB::Query_MariaDB()
{
    m_stmt = nullptr;
    m_bindedInputParams = nullptr;
    m_bindedResultsParams = nullptr;

    m_databaseConnectionHandler = nullptr;
}

Query_MariaDB::~Query_MariaDB()
{
    // Destroy binded values.
    for (size_t pos=0;pos<m_keysByPos.size();pos++)
    {
        if (m_bindedInputParams[pos].buffer)
        {
            if (m_bindedInputParams[pos].buffer_type == MYSQL_TYPE_LONGLONG && m_bindedInputParams[pos].is_unsigned)
            {
                if (m_bindedInputParams[pos].is_unsigned)
                {
                    unsigned long long * buffer = (unsigned long long *)m_bindedInputParams[pos].buffer;
                    delete buffer;
                }
                else
                {
                    long long * buffer = (long long *)m_bindedInputParams[pos].buffer;
                    delete buffer;
                }
            }
            else if (m_bindedInputParams[pos].buffer_type == MYSQL_TYPE_DOUBLE)
            {
                double * buffer = (double *)m_bindedInputParams[pos].buffer;
                delete buffer;
            }
            else if (m_bindedInputParams[pos].buffer_type == MYSQL_TYPE_LONG)
            {
                if (m_bindedInputParams[pos].is_unsigned)
                {
                    unsigned long * buffer = (unsigned long *)m_bindedInputParams[pos].buffer;
                    delete buffer;
                }
                else
                {
                    long * buffer = (long *)m_bindedInputParams[pos].buffer;
                    delete buffer;
                }
            }
            else if (m_bindedInputParams[pos].buffer_type == MYSQL_TYPE_TINY)
            {
                if (m_bindedInputParams[pos].is_unsigned)
                {
                    unsigned char * buffer = (unsigned char *)m_bindedInputParams[pos].buffer;
                    delete buffer;
                }
                else
                {
                    char * buffer = (char *)m_bindedInputParams[pos].buffer;
                    delete buffer;
                }
            }
            else if (m_bindedInputParams[pos].buffer_type == MYSQL_TYPE_SHORT)
            {
                if (m_bindedInputParams[pos].is_unsigned)
                {
                    unsigned short * buffer = (unsigned short *)m_bindedInputParams[pos].buffer;
                    delete buffer;
                }
                else
                {
                    short * buffer = (short *)m_bindedInputParams[pos].buffer;
                    delete buffer;
                }
            }

        }
    }

    // Destroy main items:
    if (m_bindedInputParams) delete [] m_bindedInputParams;
    if (m_bindedResultsParams) delete [] m_bindedResultsParams;

    // Destroy the statement
    if (m_stmt)
    {
        mysql_stmt_free_result(m_stmt);
        mysql_stmt_close(m_stmt);
        m_stmt = NULL;
    }
}
bool Query_MariaDB::step0()
{
    bool r = mysql_stmt_fetch (m_stmt) == 0;

    if (!r)
        return false;

    // Now bind each variable.
    for ( size_t col=0; col<m_resultVars.size(); col++ )
    {
        Memory::Abstract::Var * val = m_resultVars[col];

        Memory::Abstract::BINARY::sBinContainer * sBin = nullptr;

        unsigned char cRetBoolean=0;
        MYSQL_BIND result = {};
        my_bool bIsNull;

        ZeroBStruct(result);
        result.is_null = &bIsNull;

        switch (m_resultVars[col]->getVarType())
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
        mysql_stmt_fetch_column(m_stmt, &result, col, 0);

        if ( m_resultVars[col]->getVarType() == Memory::Abstract::Var::TYPE_BOOL )
        {
            ABSTRACT_PTR_AS(BOOL,val)->setValue(cRetBoolean);
        }

        if (sBin)
        {
            switch (m_resultVars[col]->getVarType())
            {
            case Memory::Abstract::Var::TYPE_BIN:
            {
                ABSTRACT_PTR_AS(BINARY,m_resultVars[col])->setValue( sBin );
            }break;
            case Memory::Abstract::Var::TYPE_STRING:
            {
                ABSTRACT_PTR_AS(STRING,m_resultVars[col])->setValue( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_STRINGLIST:
            {
                ABSTRACT_PTR_AS(STRINGLIST,m_resultVars[col])->fromString( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_DATETIME:
            {
                ABSTRACT_PTR_AS(DATETIME,m_resultVars[col])->fromString( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_IPV4:
            {
                ABSTRACT_PTR_AS(IPV4,m_resultVars[col])->fromString( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_MACADDR:
            {
                ABSTRACT_PTR_AS(MACADDR,m_resultVars[col])->fromString( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_IPV6:
            {
                ABSTRACT_PTR_AS(IPV6,m_resultVars[col])->fromString( sBin->ptr );
            }break;
            case Memory::Abstract::Var::TYPE_PTR:
            {
                ABSTRACT_PTR_AS(PTR,m_resultVars[col])->setValue((void *)createDestroyableStringForResults(sBin->ptr)->c_str());
            }break;
            default:
                break;
            }

            delete sBin;
        }

        m_fieldIsNull.push_back(bIsNull);
    }

    return true;
}

void Query_MariaDB::mariaDBSetDatabaseConnector(MYSQL *dbCnt)
{
    this->m_databaseConnectionHandler = dbCnt;
}

bool Query_MariaDB::postBindInputVars()
{
    // Load Keys:
    std::list<std::string> keysIn;
    for (auto & i : m_inputVars) keysIn.push_back(i.first);

    // Replace the keys for ?:
    while (replaceFirstKey(m_query,keysIn,m_keysByPos, "?"))
    {}

    if (!m_keysByPos.size())
        return true;

    // Create the bind struct...
    m_bindedInputParams = new MYSQL_BIND[m_keysByPos.size()];

    for (size_t pos=0; pos<m_keysByPos.size(); pos++)
    {
        ZeroBStruct(m_bindedInputParams[pos]);

        m_bindedInputParams[pos].is_unsigned = 0;
        m_bindedInputParams[pos].length = 0;

        /*
        Bind params here.
        */
        switch (m_inputVars[ m_keysByPos[pos] ]->getVarType())
        {
        case Memory::Abstract::Var::TYPE_BOOL:
        {
            unsigned char * buffer = new unsigned char;
            buffer[0] = ABSTRACT_PTR_AS(BOOL,m_inputVars[ m_keysByPos[pos] ])->getValue()?1:0;

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_TINY;
            m_bindedInputParams[pos].buffer = (char *) buffer;
            m_bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::Var::TYPE_INT8:
        {
            char * buffer = new char;
            (*buffer) = ABSTRACT_PTR_AS(INT8,m_inputVars[ m_keysByPos[pos] ])->getValue();

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_TINY;
            m_bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::Var::TYPE_INT16:
        {
            short * buffer = new short;
            (*buffer) = ABSTRACT_PTR_AS(INT16,m_inputVars[ m_keysByPos[pos] ])->getValue();

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_SHORT;
            m_bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::Var::TYPE_INT32:
        {
            long * buffer = new long;
            (*buffer) = ABSTRACT_PTR_AS(INT32,m_inputVars[ m_keysByPos[pos] ])->getValue();

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONG;
            m_bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::Var::TYPE_INT64:
        {
            long long * buffer = new long long;
            (*buffer) = ABSTRACT_PTR_AS(INT64,m_inputVars[ m_keysByPos[pos] ])->getValue();

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONGLONG;
            m_bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::Var::TYPE_UINT8:
        {
            unsigned char * buffer = new unsigned char;
            (*buffer) = ABSTRACT_PTR_AS(UINT8,m_inputVars[ m_keysByPos[pos] ])->getValue();

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_TINY;
            m_bindedInputParams[pos].buffer = (char *) buffer;
            m_bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::Var::TYPE_UINT16:
        {
            unsigned short * buffer = new unsigned short;
            (*buffer) = ABSTRACT_PTR_AS(UINT16,m_inputVars[ m_keysByPos[pos] ])->getValue();

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_SHORT;
            m_bindedInputParams[pos].buffer = (char *) buffer;
            m_bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::Var::TYPE_UINT32:
        {
            unsigned long * buffer = new unsigned long;
            (*buffer) = ABSTRACT_PTR_AS(UINT32,m_inputVars[ m_keysByPos[pos] ])->getValue();

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONG;
            m_bindedInputParams[pos].buffer = (char *) buffer;
            m_bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::Var::TYPE_UINT64:
        {
            unsigned long long * buffer = new unsigned long long;
            (*buffer) = ABSTRACT_PTR_AS(UINT64,m_inputVars[ m_keysByPos[pos] ])->getValue();

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONGLONG;
            m_bindedInputParams[pos].buffer = (char *) buffer;
            m_bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::Var::TYPE_DOUBLE:
        {
            double * buffer = new double;
            (*buffer) = ABSTRACT_PTR_AS(DOUBLE,m_inputVars[ m_keysByPos[pos] ])->getValue();

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_DOUBLE;
            m_bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::Var::TYPE_BIN:
        {
            Memory::Abstract::BINARY::sBinContainer * i = ABSTRACT_PTR_AS(BINARY,m_inputVars[ m_keysByPos[pos] ])->getValue();
            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_BLOB;
            m_bindedInputParams[pos].buffer_length = i->dataSize;
            m_bindedInputParams[pos].buffer = (char *) i->ptr;
        } break;
        case Memory::Abstract::Var::TYPE_VARCHAR:
        {
            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_STRING;
            m_bindedInputParams[pos].buffer_length = strnlen(ABSTRACT_PTR_AS(VARCHAR,m_inputVars[ m_keysByPos[pos] ])->getValue(),ABSTRACT_PTR_AS(VARCHAR,m_inputVars[ m_keysByPos[pos] ])->getVarSize())+1;
            m_bindedInputParams[pos].buffer = (char *) ABSTRACT_PTR_AS(VARCHAR,m_inputVars[ m_keysByPos[pos] ])->getValue();
        } break;
        case Memory::Abstract::Var::TYPE_PTR:
        {
            void * ptr = ABSTRACT_PTR_AS(PTR,m_inputVars[ m_keysByPos[pos] ])->getValue();
            // Threat PTR as char * (be careful, we should receive strlen compatible string, without null termination will result in an undefined behaviour)
            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_STRING;
            m_bindedInputParams[pos].buffer_length = strnlen((char *)ptr,0xFFFFFFFF);
            m_bindedInputParams[pos].buffer = (char *) ptr;
        } break;

        case Memory::Abstract::Var::TYPE_STRING:
        case Memory::Abstract::Var::TYPE_STRINGLIST:
        case Memory::Abstract::Var::TYPE_DATETIME:
        case Memory::Abstract::Var::TYPE_MACADDR:
        case Memory::Abstract::Var::TYPE_IPV4:
        case Memory::Abstract::Var::TYPE_IPV6:
        {
            std::string * str = nullptr;

            switch (m_inputVars[ m_keysByPos[pos] ]->getVarType())
            {
            case Memory::Abstract::Var::TYPE_STRING:
            {
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(STRING,m_inputVars[ m_keysByPos[pos] ])->getValue());
            } break;
            case Memory::Abstract::Var::TYPE_STRINGLIST:
            {
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(STRINGLIST,m_inputVars[ m_keysByPos[pos] ])->toString());
            } break;
            case Memory::Abstract::Var::TYPE_DATETIME:
            {
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(DATETIME,m_inputVars[ m_keysByPos[pos] ])->toString());
            } break;
            case Memory::Abstract::Var::TYPE_IPV4:
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(IPV4,m_inputVars[ m_keysByPos[pos] ])->toString());
                break;
            case Memory::Abstract::Var::TYPE_MACADDR:
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(MACADDR,m_inputVars[ m_keysByPos[pos] ])->toString());
                break;
            case Memory::Abstract::Var::TYPE_IPV6:
                str = createDestroyableStringForInput(ABSTRACT_PTR_AS(IPV6,m_inputVars[ m_keysByPos[pos] ])->toString());
                break;
            default:
                break;
            }

            m_bindedInputParams[pos].buffer_type = MYSQL_TYPE_STRING;

            if (str)
            {
                m_bindedInputParams[pos].buffer_length = str->size();
                m_bindedInputParams[pos].buffer = (char *) str->c_str();
            }
            else
            {
                m_bindedInputParams[pos].buffer_length = 0;
                m_bindedInputParams[pos].buffer = 0;
            }
        } break;
        case Memory::Abstract::Var::TYPE_NULL:
            m_bindedInputParams[pos].is_null_value = 1;
            break;
        }
    }

    return true;
}

int Query_MariaDB::reconnection(const ExecType &execType, bool recursion)
{
    // Reconnection:
    while ( connectionError() && !recursion )
    {
        // Reconnect here until timeout...
        if (((SQLConnector_MariaDB*)m_pSQLConnector)->reconnect(0xFFFFABCD))
        {
            // Remove the prepared statement...
            if (m_stmt)
            {
                mysql_stmt_free_result(m_stmt);
                mysql_stmt_close(m_stmt);
                m_stmt = NULL;
            }

            // Reconnected... executing the query again...
            bool result2 = exec0(execType,true);

            // if Resulted in another error or success
            if (!connectionError())
                return result2?1:0;

            // Otherwise, keep reconnecting...

            // ...
            if (result2 == true)
                throw std::runtime_error("how this can be true?.");
        }
        else
        {
            // Not reconnected, timed out...
            m_lastSQLError = "reconnection failed.";
            return 0;
        }
    }
    return -1;
}

bool Query_MariaDB::connectionError()
{
    return (    m_lastSQLErrno == CR_CONN_HOST_ERROR
            ||  m_lastSQLErrno == CR_SERVER_GONE_ERROR
            ||  m_lastSQLErrno == CR_CONNECTION_ERROR
                ||  m_lastSQLErrno == CR_SERVER_LOST
                ||  m_lastSQLErrno ==CR_UNKNOWN_HOST
                ||  m_lastSQLErrno == 1927
                );
}


bool Query_MariaDB::exec0(const ExecType &execType, bool recursion)
{
    m_lastSQLReturnValue = 0;
    m_lastSQLErrno = 0;

    if (m_stmt)
    {
        throw std::runtime_error("Re-using queries is not supported.");
        return false;
    }

    ((SQLConnector_MariaDB*)m_pSQLConnector)->getDatabaseConnector(this);

    if (!m_databaseConnectionHandler)
        return false;

    // Prepare the query (will lock the db while using ppDb):
    m_stmt = mysql_stmt_init(m_databaseConnectionHandler);
    if (m_stmt==nullptr)
    {
        return false;
    }

    /////////////////
    // Prepare the statement
    if ((m_lastSQLReturnValue = mysql_stmt_prepare(m_stmt, m_query.c_str(), m_query.size())) != 0)
    {
        m_lastSQLErrno = mysql_stmt_errno(m_stmt);
        int i=0;
        if ((i=reconnection(execType,recursion))>=0)
            return i==1?true:false;

        m_lastSQLError = mysql_stmt_error(m_stmt);
        return false;
    }

    ////////////////
    // Count/convert the parameters into "?"
    // Now we have an ordered array with the keys:
    if (mysql_stmt_bind_param(m_stmt, m_bindedInputParams))
    {
        m_lastSQLError = mysql_stmt_error(m_stmt);
        return false;
    }

    ///////////////
    // Execute!!
    if ((m_lastSQLReturnValue = mysql_stmt_execute(m_stmt)) != 0)
    {
        m_lastSQLErrno = mysql_stmt_errno(m_stmt);
        int i=0;
        if ((i=reconnection(execType,recursion))>=0)
            return i==1?true:false;

        // When failed with another error:
        m_lastSQLError = mysql_stmt_error(m_stmt);
        return false;
    }

    m_numRows=0;
    m_affectedRows=0;

    if(mysql_stmt_store_result(m_stmt)!=0)
    {
        m_lastSQLError = mysql_stmt_error(m_stmt);
        return false;
    }

    ///////////////
    if (execType != EXEC_TYPE_SELECT)
    {
        if (m_fetchLastInsertRowID)
            m_lastInsertRowID = mysql_stmt_insert_id(m_stmt);
        m_affectedRows = mysql_stmt_num_rows(m_stmt);
    }
    else
    {
        m_numRows = mysql_stmt_num_rows(m_stmt);
    }

    return true;
}

unsigned long Query_MariaDB::mariaDBfetchVarSize(const size_t &col, const enum_field_types & fieldType)
{
    unsigned long r=0;

    my_bool isTruncated = 0;
    char aBuffer[64];

    MYSQL_BIND bind = {};
    bind.buffer_type = fieldType;
    bind.buffer = aBuffer;
    bind.buffer_length = 64;
    bind.is_null = 0;
    bind.length = &r;
    bind.error = &isTruncated;

    if (mysql_stmt_fetch_column(m_stmt, &bind, col, 0)==0)
        return r;
    else
        return 0;
}

