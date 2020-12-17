#include "query_mariadb.h"
#include "sqlconnector_mariadb.h"
#include <string.h>
#include <cx2_mem_vars/a_allvars.h>

using namespace CX2::Database;

Query_MariaDB::Query_MariaDB()
{
    bDeallocateResultSet = false;
    stmt = nullptr;
    bindedParams = nullptr;
    bindedResults = nullptr;
    bIsNull = nullptr;
}

Query_MariaDB::~Query_MariaDB()
{
    // Destroy strings.
    for (auto * i : destroyableStrings) delete i;

    // Destroy binded values.
    for (size_t pos=0;pos<keysByPos.size();pos++)
    {
        if (bindedParams[pos].buffer_type == MYSQL_TYPE_LONGLONG)
        {
            unsigned long long * buffer = (unsigned long long *)bindedParams[pos].buffer;
            delete buffer;
        }
        else if (bindedParams[pos].buffer_type == MYSQL_TYPE_DOUBLE)
        {
            double * buffer = (double *)bindedParams[pos].buffer;
            delete buffer;
        }
        else if (bindedParams[pos].buffer_type == MYSQL_TYPE_LONG)
        {
            long * buffer = (long *)bindedParams[pos].buffer;
            delete buffer;
        }
    }

    // Destroy main items:
    if (bIsNull) delete [] bIsNull;
    if (bindedParams) delete [] bindedParams;
    if (bindedResults) delete [] bindedResults;

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

    // Prepare the query (will lock the db while using ppDb):
    if (!((SQLConnector_MariaDB*)sqlConnector)->prepareQuery(this))
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
    if (mysql_stmt_bind_param(stmt, bindedParams))
    {
        lastSQLError = mysql_stmt_error(stmt);
        return false;
    }

    ///////////////
    if (execType == EXEC_TYPE_SELECT)
    {
        if (mysql_stmt_bind_result(stmt, bindedResults))
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
            if (bFetchInsertRowID)
                 lastInsertRowID = mysql_stmt_insert_id(stmt);
        }
        else
        {
            lastSQLError = mysql_stmt_error(stmt);
            return false;
        }
    }

    return true;
}

bool Query_MariaDB::step()
{
    return mysql_stmt_fetch (stmt) == 0;
}

bool Query_MariaDB::mariadbInitSTMT(MYSQL *dbCnt)
{
    stmt = mysql_stmt_init(dbCnt);
    return stmt!=nullptr;
}

bool Query_MariaDB::replaceFirstKey(std::string &sqlQuery, std::list<std::string> &keysIn, std::vector<std::string> &keysOutByPos)
{
    std::list<std::string> toDelete;

    // Check who is the first key.
    std::size_t firstKeyPos = std::string::npos;
    std::string firstKeyFound;

    for ( auto & key : keysIn )
    {
        std::size_t pos = sqlQuery.find(key);
        if (pos!=std::string::npos)
        {
            if (pos <= firstKeyPos)
            {
                firstKeyPos = pos;
                firstKeyFound = key;
            }
        }
        else
            toDelete.push_back( key );
    }

    // not used needles will be deleted.
    for ( auto & needle : toDelete )
        keysIn.remove(needle);

    // If there is a key, replace.
    if (firstKeyPos!=std::string::npos)
    {
        keysOutByPos.push_back(firstKeyFound);
        sqlQuery.replace(firstKeyPos, firstKeyFound.length(), "?");
        return true;
    }
    return false;
}

bool Query_MariaDB::postBindInputVars()
{
    // Load Keys:
    std::list<std::string> keysIn;
    for (auto & i : InputVars) keysIn.push_back(i.first);

    // Replace the keys for ?:
    while (replaceFirstKey(query,keysIn,keysByPos))
    {}

    if (!keysByPos.size())
        return true;

    // Create the bind struct...
    bindedParams = new MYSQL_BIND[keysByPos.size()];
    bIsNull = new my_bool[keysByPos.size()];

    for (size_t pos=0; pos<keysByPos.size(); pos++)
    {
        memset(&(bindedParams[pos]),0,sizeof(MYSQL_BIND));

        bIsNull[pos] = 0;

        bindedParams[pos].is_null = &(bIsNull[pos]);
        bindedParams[pos].is_unsigned = 0;
        bindedParams[pos].length = 0;

        /*
        Bind params here.
        */
        switch (InputVars[ keysByPos[pos] ].getVarType())
        {
        case Memory::Abstract::TYPE_BOOL:
        {
            long * buffer = new long;
            (*buffer) = ABSTRACT_AS(BOOL,InputVars[ keysByPos[pos] ])->getValue()?1:0;

            bindedParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedParams[pos].buffer = (char *) buffer;

        } break;
        case Memory::Abstract::TYPE_INT8:
        {
            long * buffer = new long;
            (*buffer) = ABSTRACT_AS(INT8,InputVars[ keysByPos[pos] ])->getValue();

            bindedParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::TYPE_INT16:
        {
            long * buffer = new long;
            (*buffer) = ABSTRACT_AS(INT16,InputVars[ keysByPos[pos] ])->getValue();

            bindedParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::TYPE_INT32:
        {
            long * buffer = new long;
            (*buffer) = ABSTRACT_AS(INT32,InputVars[ keysByPos[pos] ])->getValue();

            bindedParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::TYPE_INT64:
        {
            long long * buffer = new long long;
            (*buffer) = ABSTRACT_AS(INT64,InputVars[ keysByPos[pos] ])->getValue();

            bindedParams[pos].buffer_type = MYSQL_TYPE_LONGLONG;
            bindedParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::TYPE_UINT8:
        {
            unsigned long * buffer = new unsigned long;
            (*buffer) = ABSTRACT_AS(UINT8,InputVars[ keysByPos[pos] ])->getValue();

            bindedParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedParams[pos].buffer = (char *) buffer;
            bindedParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::TYPE_UINT16:
        {
            unsigned long * buffer = new unsigned long;
            (*buffer) = ABSTRACT_AS(UINT16,InputVars[ keysByPos[pos] ])->getValue();

            bindedParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedParams[pos].buffer = (char *) buffer;
            bindedParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::TYPE_UINT32:
        {
            unsigned long * buffer = new unsigned long;
            (*buffer) = ABSTRACT_AS(UINT32,InputVars[ keysByPos[pos] ])->getValue();

            bindedParams[pos].buffer_type = MYSQL_TYPE_LONG;
            bindedParams[pos].buffer = (char *) buffer;
            bindedParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::TYPE_UINT64:
        {
            unsigned long long * buffer = new unsigned long long;
            (*buffer) = ABSTRACT_AS(UINT64,InputVars[ keysByPos[pos] ])->getValue();

            bindedParams[pos].buffer_type = MYSQL_TYPE_LONGLONG;
            bindedParams[pos].buffer = (char *) buffer;
            bindedParams[pos].is_unsigned = 1;
        } break;
        case Memory::Abstract::TYPE_DOUBLE:
        {
            double * buffer = new double;
            (*buffer) = ABSTRACT_AS(DOUBLE,InputVars[ keysByPos[pos] ])->getValue();

            bindedParams[pos].buffer_type = MYSQL_TYPE_DOUBLE;
            bindedParams[pos].buffer = (char *) buffer;
        } break;
        case Memory::Abstract::TYPE_BIN:
        {
            Memory::Abstract::sBinContainer * i = ABSTRACT_AS(BINARY,InputVars[ keysByPos[pos] ])->getValue();
            bindedParams[pos].buffer_type = MYSQL_TYPE_BLOB;
            bindedParams[pos].buffer_length = i->dataSize;
            bindedParams[pos].buffer = (char *) i->ptr;
        } break;
        case Memory::Abstract::TYPE_VARCHAR:
        {
            bindedParams[pos].buffer_type = MYSQL_TYPE_STRING;
            bindedParams[pos].buffer_length = strnlen(ABSTRACT_AS(VARCHAR,InputVars[ keysByPos[pos] ])->getValue(),ABSTRACT_AS(VARCHAR,InputVars[ keysByPos[pos] ])->getVarSize())+1;
            bindedParams[pos].buffer = (char *) ABSTRACT_AS(VARCHAR,InputVars[ keysByPos[pos] ])->getValue();
        } break;
        case Memory::Abstract::TYPE_PTR:
        {
            void * ptr = ABSTRACT_AS(PTR,InputVars[ keysByPos[pos] ])->getValue();
            // Threat PTR as char * (be careful, we should receive strlen compatible string, without null termination will result in an undefined behaviour)
            bindedParams[pos].buffer_type = MYSQL_TYPE_STRING;
            bindedParams[pos].buffer_length = strnlen((char *)ptr,0xFFFFFFFF);
            bindedParams[pos].buffer = (char *) ptr;
        } break;

        case Memory::Abstract::TYPE_STRING:
        case Memory::Abstract::TYPE_STRINGLIST:
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
            case Memory::Abstract::TYPE_IPV4:
                str = createDestroyableString(ABSTRACT_AS(IPV4,InputVars[ keysByPos[pos] ])->toString());
                break;
            case Memory::Abstract::TYPE_IPV6:
                str = createDestroyableString(ABSTRACT_AS(IPV6,InputVars[ keysByPos[pos] ])->toString());
                break;
            default:
                break;
            }

            // TODO: destroy i
            bindedParams[pos].buffer_type = MYSQL_TYPE_STRING;
            bindedParams[pos].buffer_length = str->size()+1;
            bindedParams[pos].buffer = (char *) str->c_str();
        }
        case  Memory::Abstract::TYPE_NULL:
            bIsNull[pos] = 1;
            break;
        }
    }

    return true;
}

bool Query_MariaDB::postBindResultVars()
{
    if (!resultVars.size())
        return true;

    bindedResults = new MYSQL_BIND[resultVars.size()];

    long i=0;
    for (auto * val : resultVars)
    {
        memset(&(bindedResults[i]),0,sizeof(MYSQL_BIND));

        switch (val->getVarType())
        {
        case Memory::Abstract::TYPE_BOOL:
            throw std::runtime_error("BOOL return type in MySQL is not compatible, please use INT32.");
            break;
        case Memory::Abstract::TYPE_INT8:
            throw std::runtime_error("INT8 return type in MySQL is not compatible, please use INT32.");
            break;
        case Memory::Abstract::TYPE_INT16:
            throw std::runtime_error("INT16 return type in MySQL is not compatible, please use INT32.");
            break;
        case Memory::Abstract::TYPE_INT32:
            bindedResults[i].buffer_type = MYSQL_TYPE_LONG;
            bindedResults[i].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_INT64:
            bindedResults[i].buffer_type = MYSQL_TYPE_LONGLONG;
            bindedResults[i].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_UINT8:
            throw std::runtime_error("UINT8 return type in MySQL is not compatible, please use UINT32.");
            break;
        case Memory::Abstract::TYPE_UINT16:
            throw std::runtime_error("UINT16 return type in MySQL is not compatible, please use UINT32.");
            break;
        case Memory::Abstract::TYPE_UINT32:
            bindedResults[i].buffer_type = MYSQL_TYPE_LONG;
            bindedResults[i].is_unsigned = 1;
            bindedResults[i].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_UINT64:
            bindedResults[i].buffer_type = MYSQL_TYPE_LONGLONG;
            bindedResults[i].is_unsigned = 1;
            bindedResults[i].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_DOUBLE:
            bindedResults[i].buffer_type = MYSQL_TYPE_DOUBLE;
            bindedResults[i].is_unsigned = 1;
            bindedResults[i].buffer = val->getDirectMemory();
            break;
        case Memory::Abstract::TYPE_BIN:
        {
            // TODO: how to bind this..
            throw std::runtime_error("BIN/BLOB return type is not compatible in MySQL yet, please use VARCHAR.");
        } break;
        case Memory::Abstract::TYPE_VARCHAR:
        {
            // This will copy the memory.
            bindedResults[i].buffer_type = MYSQL_TYPE_STRING;
            bindedResults[i].length = ABSTRACT_PTR_AS(VARCHAR,val)->getFillSizePTR();
            bindedResults[i].buffer = val->getDirectMemory();
            bindedResults[i].buffer_length = ABSTRACT_PTR_AS(VARCHAR,val)->getVarSize();
        } break;
        case Memory::Abstract::TYPE_STRING:
        {
            throw std::runtime_error("STRING return type is not compatible in MySQL yet, please use VARCHAR.");
        }break;
        case  Memory::Abstract::TYPE_STRINGLIST:
        {
            throw std::runtime_error("STRINGLIST return type is not compatible in MySQL yet, please use VARCHAR.");
        }break;
        case  Memory::Abstract::TYPE_IPV4:
        {
            throw std::runtime_error("IPV4 return type is not compatible in MySQL yet, please use VARCHAR.");
        }break;
        case  Memory::Abstract::TYPE_IPV6:
        {
            throw std::runtime_error("IPV6 return type is not compatible in MySQL yet, please use VARCHAR.");
        }break;
        case  Memory::Abstract::TYPE_PTR:
        {
            throw std::runtime_error("Pointer return type is not compatible in MySQL yet, please use VARCHAR.");
        } break;
        case  Memory::Abstract::TYPE_NULL:
            // Don't copy the value (not needed).
            throw std::runtime_error("NULL return type is not compatible in MySQL yet.");
            break;
        }
        i++;
    }
    return true;
}


std::string *Query_MariaDB::createDestroyableString(const std::string &str)
{
    std::string * i = new std::string;
    *i = str;
    destroyableStrings.push_back(i);
    return i;
}

my_ulonglong Query_MariaDB::getLastInsertRowID() const
{
    return lastInsertRowID;
}

bool Query_MariaDB::getFetchInsertRowID() const
{
    return bFetchInsertRowID;
}

void Query_MariaDB::setFetchInsertRowID(bool value)
{
    bFetchInsertRowID = value;
}

