#include "datatables.h"
#include <regex>

using namespace Mantids30;

static std::string getColumnNameFromColumnPos(const boost::json::value &dataTablesFilters, const uint32_t &pos)
{
    if (Helpers::JSON::ISARRAY(dataTablesFilters, "columns"))
    {
        const boost::json::value &columns = Helpers::JSON::detail::safeGet(dataTablesFilters, "columns");
        if (pos >= static_cast<uint32_t>(columns.as_array().size()))
        {
            return "";
        }
        return Helpers::JSON::ASSTRING(columns.as_array()[pos], "data", "");
    }
    return "";
}

std::string Mantids30::Helpers::DataTables::getOrderByStatement(const boost::json::value &dataTablesFilters)
{
    std::string orderByStatement;
    // Manejo de ordenamiento (order)
    const boost::json::value &orderArray = Helpers::JSON::detail::safeGet(dataTablesFilters, "order");
    if (Helpers::JSON::ISARRAY_D(orderArray) && !orderArray.as_array().empty())
    {
        const boost::json::value &orderArrayElement = orderArray.as_array()[0];
        std::string columnName = getColumnNameFromColumnPos(dataTablesFilters, Helpers::JSON::ASUINT(orderArrayElement, "column", 0));
        std::string dir = Helpers::JSON::ASSTRING(orderArrayElement, "dir", "desc");

        // Validar que el nombre del campo sea seguro (solo letras, números y guiones bajos)
        static const std::regex fieldRegex("^[a-zA-Z0-9_]+$");
        if (std::regex_match(columnName, fieldRegex))
        {
            orderByStatement = "`" + columnName + "` ";
            orderByStatement += (dir == "desc") ? "DESC" : "ASC";
        }
    }
    return orderByStatement;
}
