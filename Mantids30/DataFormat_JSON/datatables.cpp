#include "datatables.h"
#include "json_detail.h"
#include <regex>

using namespace Mantids30;

static std::string_view getColumnNameFromColumnPos(const boost::json::value &dataTablesFilters, const uint32_t &pos)
{
    const boost::json::array &columns = Helpers::JSON::ASARRAY_OR_EMPTY(dataTablesFilters, "columns");

    if (pos >= columns.size())
    {
        return "";
    }

    return Helpers::JSON::ASSTRING_D(Helpers::JSON::MEMBER_OR_NULL(columns[pos], "data"), "");
}

std::string Helpers::DataTables::getOrderByStatement(const boost::json::value &dataTablesFilters)
{
    std::string orderByStatement;

    const boost::json::array &orderArray = Helpers::JSON::ASARRAY_OR_EMPTY(dataTablesFilters, "order");

    if (!orderArray.empty())
    {
        const boost::json::value &orderElement = orderArray[0];

        const std::string_view columnName = getColumnNameFromColumnPos(dataTablesFilters, Helpers::JSON::ASUINT(orderElement, "column", 0));
        const std::string_view dir = Helpers::JSON::ASSTRING(orderElement, "dir", "desc");

        static const std::regex fieldRegex("^[a-zA-Z0-9_]+$");

        if (std::regex_match(columnName.begin(), columnName.end(), fieldRegex))
        {
            orderByStatement = "`" + std::string(columnName) + "` " + (dir == "desc" ? "DESC" : "ASC");
        }
    }

    return orderByStatement;
}
