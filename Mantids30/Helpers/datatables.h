#pragma once

#include "json.h"

namespace Mantids30::Helpers {

class DataTables
{
public:
    static std::string getOrderByStatement(const json &dataTablesFilters);
};

} // namespace Mantids30::Helpers
