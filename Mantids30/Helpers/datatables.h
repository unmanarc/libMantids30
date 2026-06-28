#pragma once

#include "json.h"

namespace Mantids30::Helpers {

class DataTables
{
public:
    static std::string getOrderByStatement(const Json::Value &dataTablesFilters);
};

} // namespace Mantids30::Helpers
