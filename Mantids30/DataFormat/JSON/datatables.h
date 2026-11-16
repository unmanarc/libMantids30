#pragma once

#include "json.h"

namespace Mantids30::DataFormat::JSON {

class DataTables
{
public:
    static std::string getOrderByStatement(const boost::json::value &dataTablesFilters);
};

} // namespace Mantids30::DataFormat::JSON
