#include "query_mariadb.h"
#include "sqlconnector_mariadb.h"
#include <string.h>
#include <cx2_mem_vars/a_allvars.h>

using namespace CX2::Database;

Query_MariaDB::Query_MariaDB()
{
    bDeallocateResultSet = false;
    stmt = nullptr;
    bindedInputParams = nullptr;
    bindedResultsParams = nullptr;
    bIsNull = nullptr;
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
    if (bIsNull) delete [] bIsNull;
    if (bindedInputParams) delete [] bindedInputParams;
    if (bindedResultsParams) delete [] bindedResultsParams;

    // Destroy result set.
    if (bDeallocateResultSet)
        mysql_stmt_free_result(stmt);

    // Destroy the statement
    if (stmt)
    {
        mysql_stmt_close(stmt);
        stmt = NULL;
    }
}

bool Query_MariaDB::exec(const ExecType &execType)
{
    if (stmt)
    {
        throw std::runtime_error("Re-using queries is not supported.");
        return false;
    }

    ((SQLConnector_MariaDB*)sqlConnector)->getDatabaseConnector(this);

    std::unique_lock<std::mutex> lock(*mtDatabaseLock);

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
    if (execType == EXEC_TYPE_SELECT)
    {
        if (mysql_stmt_bind_result(stmt, bindedResultsParams))
        {
            lastSQLError = mysql_stmt_error(stmt);
            return false;
        }
    }

    ///////////////
    // Execute!!
    if ((lastSQLReturnValue = mysql_stmt_execute(stmt)) != 0)
    {
        lastSQLError = mysql_stmt_error(stmt);
        return false;
    }

    ///////////////
    if (execType == EXEC_TYPE_SELECT)
    {
        // Store the results:
        if ((lastSQLReturnValue = mysql_stmt_store_result(stmt)) == 0)
        {
            bDeallocateResultSet = true;
        }
        else
        {
            lastSQLError = mysql_stmt_error(stmt);
            return false;
        }
    }
    else
    {
        if (bFetchLastInsertRowID)
            lastInsertRowID = mysql_stmt_insert_id(stmt);
    }

    return true;
}

bool Query_MariaDB::step()
{
    bool r = mysql_stmt_fetch (stmt) == 0;
    std::map<size_t,Memory::Abstract::VARCHAR *> toGet;

    // Now fetch the good variables and check sizes.

    if (r)
    {
        for ( size_t col=0; col<resultVars.size(); col++ )
        {
            isNull.push_back( bIsNull[col] );

            switch (resultVars[col]->getVarType())
            {
            case Memory::Abstract::TYPE_BOOL:
            {
                // Fetch the column here..
                Memory::Abstract::VARCHAR * val = new Memory::Abstract::VARCHAR(1);
                bindedResultsParams[col].buffer_type = MYSQL_TYPE_TINY;
                bindedResultsParams[col].is_unsigned = 1;
                bindedResultsParams[col].buffer = val->getDirectMemory();
                toGet[col] = val;
            }break;
            case Memory::Abstract::TYPE_BIN:
            {
                // Fetch the column here..
                Memory::Abstract::VARCHAR * val = new Memory::Abstract::VARCHAR(bindedResultsParams[col].buffer_length);
                bindedResultsParams[col].buffer_type = MYSQL_TYPE_BLOB;
                bindedResultsParams[col].buffer = val->getDirectMemory();
                toGet[col] = val;
            }break;
            case Memory::Abstract::TYPE_STRING:
            {
                // Fetch the column here..
                Memory::Abstract::VARCHAR * val = new Memory::Abstract::VARCHAR(bindedResultsParams[col].buffer_length);
                bindedResultsParams[col].buffer_type = MYSQL_TYPE_STRING;
                bindedResultsParams[col].buffer = val->getDirectMemory();
                toGet[col] = val;
            }break;
            case Memory::Abstract::TYPE_STRINGLIST:
            {
                // Fetch the column here..
                Memory::Abstract::VARCHAR * val = new Memory::Abstract::VARCHAR(bindedResultsParams[col].buffer_length);
                bindedResultsParams[col].buffer_type = MYSQL_TYPE_STRING;
                bindedResultsParams[col].buffer = val->getDirectMemory();
                toGet[col] = val;
            }break;
            case Memory::Abstract::TYPE_DATETIME:
            {
                // Fetch the column here..
                Memory::Abstract::VARCHAR * val = new Memory::Abstract::VARCHAR(bindedResultsParams[col].buffer_length);
                bindedResultsParams[col].buffer_type = MYSQL_TYPE_STRING;
                bindedResultsParams[col].buffer = val->getDirectMemory();
                toGet[col] = val;
            }break;
            case Memory::Abstract::TYPE_IPV4:
            {
                // Fetch the column here..
                Memory::Abstract::VARCHAR * val = new Memory::Abstract::VARCHAR(bindedResultsParams[col].buffer_length);
                bindedResultsParams[col].buffer_type = MYSQL_TYPE_STRING;
                bindedResultsParams[col].buffer = val->getDirectMemory();
                toGet[col] = val;
            }break;
            case Memory::Abstract::TYPE_IPV6:
            {
                // Fetch the column here..
                Memory::Abstract::VARCHAR * val = new Memory::Abstract::VARCHAR(bindedResultsParams[col].buffer_length);
                bindedResultsParams[col].buffer_type = MYSQL_TYPE_STRING;
                bindedResultsParams[col].buffer = val->getDirectMemory();
                toGet[col] = val;
            }break;
            case Memory::Abstract::TYPE_PTR:
            {
                // Fetch the column here.. (then transform to std::string :-/ very inefficient)
                Memory::Abstract::VARCHAR * val = new Memory::Abstract::VARCHAR(bindedResultsParams[col].buffer_length);
                bindedResultsParams[col].buffer_type = MYSQL_TYPE_STRING;
                bindedResultsParams[col].buffer = val->getDirectMemory();
                toGet[col] = val;
            }break;
            default:
                // Don't copy the value (not needed).
                break;
            }
        }

        // Fetch again with all values!...
        if ( !toGet.empty() )
        {
            mysql_stmt_bind_result(stmt, bindedResultsParams);
            r = mysql_stmt_fetch(stmt) == 0;
        }
    }
    if (r)
    {
        // Copy toGet Values to result vars..
        for ( const auto & i : toGet)
        {
            size_t col = i.first;
            Memory::Abstract::VARCHAR * val = i.second;

            // Set data's:
            switch (resultVars[col]->getVarType())
            {
            case Memory::Abstract::TYPE_BOOL:
            {
                ABSTRACT_PTR_AS(BOOL,resultVars[col])->setValue( val->getValue()[0]?true:false );
            }break;
            case Memory::Abstract::TYPE_BIN:
            {
                Memory::Abstract::sBinContainer x;
                x.dataSize = val->getVarSize();
                x.ptr = val->getValue();
                ABSTRACT_PTR_AS(BINARY,resultVars[col])->setValue( &x );
            }break;
            case Memory::Abstract::TYPE_STRING:
            {
                ABSTRACT_PTR_AS(STRING,resultVars[col])->setValue( val->getValue() );
            }break;
            case Memory::Abstract::TYPE_STRINGLIST:
            {
                ABSTRACT_PTR_AS(STRINGLIST,resultVars[col])->fromString( val->getValue() );
            }break;
            case Memory::Abstract::TYPE_DATETIME:
            {
                ABSTRACT_PTR_AS(DATETIME,resultVars[col])->fromString( val->getValue() );
            }break;
            case Memory::Abstract::TYPE_IPV4:
            {
                ABSTRACT_PTR_AS(IPV4,resultVars[col])->fromString( val->getValue() );
            }break;
            case Memory::Abstract::TYPE_IPV6:
            {
                ABSTRACT_PTR_AS(IPV6,resultVars[col])->fromString( val->getValue() );
            }break;
                break;
            case Memory::Abstract::TYPE_PTR:
            {
                std::string * str = createDestroyableString(val->getValue());
                ABSTRACT_PTR_AS(PTR,resultVars[col])->setValue( (void *)str->c_str() );
            }break;
            default:
                // Don't copy the value (not needed).
                break;
            }
        }
    }

    return r;
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
        memset(&(bindedInputParams[pos]),0,sizeof(MYSQL_BIND));

        bindedInputParams[pos].is_unsigned = 0;
        bindedInputParams[pos].length = 0;

        /*
        Bind params here.
        */
        switch (InputVars[ keysByPos[pos] ].getVarType())
        {
        case Memory::Abstract::TYPE_BOOL:
        {
            unsigned char * buffer = new unsigned char;
            buffer[0] = ABSTRACT_AS(BOOL,InputVars[ keysByPos[pos] ])->getValue()?1:0;

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_TINY;
            bindedInputParams[pos].buffer = (char *) buffer;
            bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::TYPE_INT8:
        {
            char * buffer = new char;
            (*buffer) = ABSTRACT_AS(INT8,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_TINY;
            bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::TYPE_INT16:
        {
            short * buffer = new short;
            (*buffer) = ABSTRACT_AS(INT16,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_SHORT;
            bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::TYPE_INT32:
        {
            long * buffer = new long;
            (*buffer) = ABSTRACT_AS(INT32,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::TYPE_INT64:
        {
            long long * buffer = new long long;
            (*buffer) = ABSTRACT_AS(INT64,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONGLONG;
            bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::TYPE_UINT8:
        {
            unsigned char * buffer = new unsigned char;
            (*buffer) = ABSTRACT_AS(UINT8,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_TINY;
            bindedInputParams[pos].buffer = (char *) buffer;
            bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::TYPE_UINT16:
        {
            unsigned short * buffer = new unsigned short;
            (*buffer) = ABSTRACT_AS(UINT16,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_SHORT;
            bindedInputParams[pos].buffer = (char *) buffer;
            bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::TYPE_UINT32:
        {
            unsigned long * buffer = new unsigned long;
            (*buffer) = ABSTRACT_AS(UINT32,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedInputParams[pos].buffer = (char *) buffer;
            bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::TYPE_UINT64:
        {
            unsigned long long * buffer = new unsigned long long;
            (*buffer) = ABSTRACT_AS(UINT64,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_LONGLONG;
            bindedInputParams[pos].buffer = (char *) buffer;
            bindedInputParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::TYPE_DOUBLE:
        {
            double * buffer = new double;
            (*buffer) = ABSTRACT_AS(DOUBLE,InputVars[ keysByPos[pos] ])->getValue();

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_DOUBLE;
            bindedInputParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::TYPE_BIN:
        {
            Memory::Abstract::sBinContainer * i = ABSTRACT_AS(BINARY,InputVars[ keysByPos[pos] ])->getValue();
            bindedInputParams[pos].buffer_type = MYSQL_TYPE_BLOB;
            bindedInputParams[pos].buffer_length = i->dataSize;
            bindedInputParams[pos].buffer = (char *) i->ptr;
        } break;
        case Memory::Abstract::TYPE_VARCHAR:
        {
            bindedInputParams[pos].buffer_type = MYSQL_TYPE_STRING;
            bindedInputParams[pos].buffer_length = strnlen(ABSTRACT_AS(VARCHAR,InputVars[ keysByPos[pos] ])->getValue(),ABSTRACT_AS(VARCHAR,InputVars[ keysByPos[pos] ])->getVarSize())+1;
            bindedInputParams[pos].buffer = (char *) ABSTRACT_AS(VARCHAR,InputVars[ keysByPos[pos] ])->getValue();
        } break;
        case Memory::Abstract::TYPE_PTR:
        {
            void * ptr = ABSTRACT_AS(PTR,InputVars[ keysByPos[pos] ])->getValue();
            // Threat PTR as char * (be careful, we should receive strlen compatible string, without null termination will result in an undefined behaviour)
            bindedInputParams[pos].buffer_type = MYSQL_TYPE_STRING;
            bindedInputParams[pos].buffer_length = strnlen((char *)ptr,0xFFFFFFFF);
            bindedInputParams[pos].buffer = (char *) ptr;
        } break;

        case Memory::Abstract::TYPE_STRING:
        case Memory::Abstract::TYPE_STRINGLIST:
        case Memory::Abstract::TYPE_DATETIME:
        case Memory::Abstract::TYPE_IPV4:
        case Memory::Abstract::TYPE_IPV6:
        {
            std::string * str = nullptr;

            switch (InputVars[ keysByPos[pos] ].getVarType())
            {
            case Memory::Abstract::TYPE_STRING:
            {
                str = createDestroyableString(ABSTRACT_AS(STRING,InputVars[ keysByPos[pos] ])->getValue());
            } break;
            case Memory::Abstract::TYPE_STRINGLIST:
            {
                str = createDestroyableString(ABSTRACT_AS(STRINGLIST,InputVars[ keysByPos[pos] ])->toString());
            } break;
            case Memory::Abstract::TYPE_DATETIME:
            {
                str = createDestroyableString(ABSTRACT_AS(DATETIME,InputVars[ keysByPos[pos] ])->toString());
            } break;
            case Memory::Abstract::TYPE_IPV4:
                str = createDestroyableString(ABSTRACT_AS(IPV4,InputVars[ keysByPos[pos] ])->toString());
                break;
            case Memory::Abstract::TYPE_IPV6:
                str = createDestroyableString(ABSTRACT_AS(IPV6,InputVars[ keysByPos[pos] ])->toString());
                break;
            default:
                break;
            }

            bindedInputParams[pos].buffer_type = MYSQL_TYPE_STRING;
            bindedInputParams[pos].buffer_length = str->size()+1;
            bindedInputParams[pos].buffer = (char *) str->c_str();
        } break;
        case Memory::Abstract::TYPE_NULL:
            bindedInputParams[pos].is_null_value = 1;
            break;
        }
    }

    return true;
}

bool Query_MariaDB::postBindResultVars()
{
    if (!resultVars.size())
        return true;

    bindedResultsParams = new MYSQL_BIND[resultVars.size()];
    bIsNull = new my_bool[resultVars.size()];

    long col=0;
    for (auto * val : resultVars)
    {
        memset(&(bindedResultsParams[col]),0,sizeof(MYSQL_BIND));

        bIsNull[col] = 0;
        bindedResultsParams[col].is_null = &(bIsNull[col]);

        switch (val->getVarType())
        {
        case Memory::Abstract::TYPE_BOOL:
            // fetch later.
            break;
        case Memory::Abstract::TYPE_INT8:
            bindedResultsParams[col].buffer_type = MYSQL_TYPE_TINY;
            bindedResultsParams[col].is_unsigned = 0;
            bindedResultsParams[col].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_INT16:
            bindedResultsParams[col].buffer_type = MYSQL_TYPE_SHORT;
            bindedResultsParams[col].is_unsigned = 0;
            bindedResultsParams[col].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_INT32:
            bindedResultsParams[col].buffer_type = MYSQL_TYPE_LONG;
            bindedResultsParams[col].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_INT64:
            bindedResultsParams[col].buffer_type = MYSQL_TYPE_LONGLONG;
            bindedResultsParams[col].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_UINT8:
            bindedResultsParams[col].buffer_type = MYSQL_TYPE_TINY;
            bindedResultsParams[col].is_unsigned = 1;
            bindedResultsParams[col].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_UINT16:
            bindedResultsParams[col].buffer_type = MYSQL_TYPE_SHORT;
            bindedResultsParams[col].is_unsigned = 1;
            bindedResultsParams[col].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_UINT32:
            bindedResultsParams[col].buffer_type = MYSQL_TYPE_LONG;
            bindedResultsParams[col].is_unsigned = 1;
            bindedResultsParams[col].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_UINT64:
            bindedResultsParams[col].buffer_type = MYSQL_TYPE_LONGLONG;
            bindedResultsParams[col].is_unsigned = 1;
            bindedResultsParams[col].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_DOUBLE:
            bindedResultsParams[col].buffer_type = MYSQL_TYPE_DOUBLE;
            bindedResultsParams[col].is_unsigned = 0;
            bindedResultsParams[col].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_BIN:
            // fetch later.
            break;
        case Memory::Abstract::TYPE_VARCHAR:
        {
            // This will copy the memory.
            bindedResultsParams[col].buffer_type = MYSQL_TYPE_STRING;
            bindedResultsParams[col].length = ABSTRACT_PTR_AS(VARCHAR,val)->getFillSizePTR();
            bindedResultsParams[col].buffer = val->getDirectMemory();
            bindedResultsParams[col].buffer_length = ABSTRACT_PTR_AS(VARCHAR,val)->getVarSize();
        } break;
        case Memory::Abstract::TYPE_STRING:
            // fetch later.
            break;
        case Memory::Abstract::TYPE_STRINGLIST:
            // fetch later.
            break;
        case Memory::Abstract::TYPE_DATETIME:
            // fetch later.
            break;
        case Memory::Abstract::TYPE_IPV4:
            // fetch later.
            break;
        case Memory::Abstract::TYPE_IPV6:
            // fetch later.
            break;
        case Memory::Abstract::TYPE_PTR:
            // fetch later.
            break;
        case Memory::Abstract::TYPE_NULL:
            // Don't copy the value (not needed).
            break;
        }
        col++;
    }
    return true;
}


