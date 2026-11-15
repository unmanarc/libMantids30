#pragma once

#include "json.h"

namespace Mantids30::DataFormats::JSON {

class DataTables
{
public:
    static std::string getOrderByStatement(const boost::json::value &dataTablesFilters);
};

} // namespace Mantids30::DataFormats::JSON
