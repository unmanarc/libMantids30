#include "datatables.h"
#include <regex>

using namespace Mantids30;

static std::string getColumnNameFromColumnPos(const json &dataTablesFilters, const uint32_t &pos)
{
    if (Helpers::JSON::ISARRAY(dataTablesFilters, "columns"))
    {
        if (pos >= dataTablesFilters["columns"].size())
        {
            return "";
        }
        return Helpers::JSON::ASSTRING(dataTablesFilters["columns"][pos], "data", "");
    }
    return "";
}

std::string Mantids30::Helpers::DataTables::getOrderByStatement(const json &dataTablesFilters)
{
    std::string orderByStatement;
    // Manejo de ordenamiento (order)
    const Json::Value &orderArray = dataTablesFilters["order"];
    if (Helpers::JSON::ISARRAY_D(orderArray) && !orderArray.empty())
    {
        const Json::Value &orderArrayElement = orderArray[0];
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
