#include "datatables.h"
#include "json_detail.h"
#include <regex>

using namespace Mantids30;

static std::string_view getColumnNameFromColumnPos(const boost::json::value &dataTablesFilters, const uint32_t &pos)
{
    const boost::json::array &columns = Mantids30::DataFormats::JSON::ASARRAY_OR_EMPTY(dataTablesFilters, "columns");

    if (pos >= columns.size())
    {
        return "";
    }

    return Mantids30::DataFormats::JSON::ASSTRING_D(Mantids30::DataFormats::JSON::MEMBER_OR_NULL(columns[pos], "data"), "");
}

std::string Mantids30::DataFormats::JSON::DataTables::getOrderByStatement(const boost::json::value &dataTablesFilters)
{
    std::string orderByStatement;

    const boost::json::array &orderArray = Mantids30::DataFormats::JSON::ASARRAY_OR_EMPTY(dataTablesFilters, "order");

    if (!orderArray.empty())
    {
        const boost::json::value &orderElement = orderArray[0];

        const std::string_view columnName = getColumnNameFromColumnPos(dataTablesFilters, Mantids30::DataFormats::JSON::ASUINT(orderElement, "column", 0));
        const std::string_view dir = Mantids30::DataFormats::JSON::ASSTRING(orderElement, "dir", "desc");

        static const std::regex fieldRegex("^[a-zA-Z0-9_]+$");

        if (std::regex_match(columnName.begin(), columnName.end(), fieldRegex))
        {
            orderByStatement = "`" + std::string(columnName) + "` " + (dir == "desc" ? "DESC" : "ASC");
        }
    }

    return orderByStatement;
}
